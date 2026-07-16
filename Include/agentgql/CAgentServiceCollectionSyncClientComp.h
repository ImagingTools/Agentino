// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QByteArrayList>
#include <QtCore/QString>

// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <imod/TSingleModelObserverBase.h>
#include <iprm/ITextParam.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtclientgql/IGqlClient.h>


namespace agentgql
{


/**
	Live agent → server service-collection sync over the existing WebSocket client link.

	Observes the local ServiceCollection (same pattern as CObjectCollectionChangeNotifierComp)
	and notifies the central server via GQL command 'NotifyAgentServicesCollectionChanged'.

	OnUpdate may run on a GQL worker thread; WebSocket SendRequest must run on the main thread.
	NotifyServer is therefore always queued to this QObject's thread.
*/
class CAgentServiceCollectionSyncClientComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			protected imod::TSingleModelObserverBase<imtbase::IObjectCollection>
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;
	typedef imod::TSingleModelObserverBase<imtbase::IObjectCollection> BaseClass2;

	static const QByteArray s_notifyCommandId;

	I_BEGIN_COMPONENT(CAgentServiceCollectionSyncClientComp);
		// Attribute name + I_ASSIGN_TO match CObjectCollectionChangeNotifierComp (working observer).
		I_ASSIGN(m_objectCollectionCompPtr, "ServiceCollection", "Local service collection to observe", true, "ServiceCollection");
		I_ASSIGN_TO(m_objectCollectionModelCompPtr, m_objectCollectionCompPtr, true);
		I_ASSIGN(m_gqlClientCompPtr, "GqlClient", "WebSocket client connected to the central server", true, "GqlClient");
		I_ASSIGN(m_clientIdCompPtr, "ClientIdParam", "Agent client-ID (sent as header clientid)", true, "ClientIdParam");
	I_END_COMPONENT;

protected:
	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

private:
	void QueueNotifyServer(const QString& typeOperation, const QByteArray& itemId, const QByteArrayList& itemIds);
	void NotifyServer(const QString& typeOperation, const QByteArray& itemId, const QByteArrayList& itemIds) const;

	I_REF(imtbase::IObjectCollection, m_objectCollectionCompPtr);
	I_REF(imod::IModel, m_objectCollectionModelCompPtr);
	I_REF(imtclientgql::IGqlClient, m_gqlClientCompPtr);
	I_REF(iprm::ITextParam, m_clientIdCompPtr);
};


} // namespace agentgql
