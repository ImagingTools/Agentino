#pragma once


// ACF includes
#include <imod/TSingleModelObserverBase.h>

// ImtCore includes
#include <imtcom/IConnectionStatusProvider.h>
#include <imtservergql/CGqlPublisherCompBase.h>


namespace agentgql
{


class CAgentConnectionSubscriberControllerComp:
			public imtservergql::CGqlPublisherCompBase,
			public imod::TSingleModelObserverBase<istd::IChangeable>
{
public:
	typedef imtservergql::CGqlPublisherCompBase BaseClass;
	typedef imod::TSingleModelObserverBase<istd::IChangeable> BaseClass2;

	I_BEGIN_COMPONENT(CAgentConnectionSubscriberControllerComp);
		I_ASSIGN(m_loginStatusProviderCompPtr, "LoginStatusProvider", "Login status provider", true, "LoginStatusProvider");
		I_ASSIGN_TO(m_loginStatusModelCompPtr, m_loginStatusProviderCompPtr, true);
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlPublisherCompBase)
	virtual bool RegisterSubscription(
				const QByteArray& subscriptionId,
				const imtgql::CGqlRequest& gqlRequest,
				const imtrest::IRequest& networkRequest,
				QString& errorMessage) override;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

protected:
	I_REF(imtcom::IConnectionStatusProvider, m_loginStatusProviderCompPtr);
	I_REF(imod::IModel, m_loginStatusModelCompPtr);
};


} // namespace agentgql


