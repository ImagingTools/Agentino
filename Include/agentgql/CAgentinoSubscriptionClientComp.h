// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <imod/TSingleModelObserverBase.h>
#include <iprm/ITextParam.h>
#include <ibase/IApplicationInfo.h>

// ImtCore includes
#include <imtclientgql/IGqlSubscriptionManager.h>
#include <imtclientgql/IGqlClient.h>
#include <imtcom/IConnectionStatusProvider.h>


namespace agentgql
{


class CAgentinoSubscriptionClientComp:
			public ilog::CLoggerComponentBase,
			public imod::CSingleModelObserverBase,
			virtual public imtclientgql::IGqlSubscriptionClient
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CAgentinoSubscriptionClientComp);
		I_REGISTER_INTERFACE(imtclientgql::IGqlSubscriptionClient);
		I_ASSIGN(m_subscriptionManagerCompPtr, "SubscriptionManager", "Subscription agent manager", false, "SubscriptionManager");
		I_ASSIGN(m_gqlClientCompPtr, "GqlClient", "GraphQl client to send a request", true, "GqlClient");
		I_ASSIGN(m_loginStatusCompPtr, "WebLoginStatus", "Web login status", true, "WebLoginStatus");
		I_ASSIGN_TO(m_webLoginStatusModelCompPtr, m_loginStatusCompPtr, true);
		I_ASSIGN(m_clientIdCompPtr, "ClientIdParam", "Parameter providing the client-ID that needs to be identified on the server", false, "ClientIdParam");
		I_ASSIGN(m_applicationInfoCompPtr, "ApplicationInfo", "Application info", true, "ApplicationInfo");
	I_END_COMPONENT;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;

	// reimplemented (imtclientgql::IGqlSubscriptionClient)
	virtual void OnResponseReceived(const QByteArray& subscriptionId, const QByteArray& subscriptionData) override;
	virtual void OnSubscriptionStatusChanged(const QByteArray& subscriptionId, const SubscriptionStatus& status, const QString& message) override;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

private:
	I_REF(imtclientgql::IGqlSubscriptionManager, m_subscriptionManagerCompPtr);
	I_REF(imtclientgql::IGqlClient, m_gqlClientCompPtr);
	I_REF(imtcom::IConnectionStatusProvider, m_loginStatusCompPtr);
	I_REF(imod::IModel, m_webLoginStatusModelCompPtr);
	I_REF(iprm::ITextParam, m_clientIdCompPtr);
	I_REF(ibase::IApplicationInfo, m_applicationInfoCompPtr);

	QByteArray m_serviceStatusSubsriptionId;
};


} // namespace agentgql


