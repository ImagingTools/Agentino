// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QFuture>
#include <QtCore/QTimer>

// ACF includes
#include <imod/TSingleModelObserverBase.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtservergql/CGqlPublisherCompBase.h>

// Agentino includes
#include <agentgql/CServiceLog.h>


namespace agentgql
{


class CServiceLogSubscriberControllerComp:
			public QObject,
			public imtservergql::CGqlPublisherCompBase,
			public imod::CSingleModelObserverBase,
			public CServiceLog
{
	Q_OBJECT
public:
	typedef imtservergql::CGqlPublisherCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceLogSubscriberControllerComp);
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Service collection used to manage services", true, "ServiceCollection");
		I_ASSIGN_TO(m_serviceCollectionModelCompPtr, m_serviceCollectionCompPtr, true);
	I_END_COMPONENT;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

protected Q_SLOTS:
	void TimerUpdate();

private:
	struct MessageStatusInfo{
		istd::TSharedInterfacePtr<imtbase::IObjectCollection> messageCollectionPtr;
		// -1 until the first poll sets the baseline
		int messageCount = -1;
	};

	typedef QMap<QByteArray, int> MessageCounts;

	void OnPollFinished(const MessageCounts& messageCounts, qint64 elapsedMs);

	static constexpr int s_minPollIntervalMs = 1000;
	static constexpr int s_maxPollIntervalMs = 60000;

	QMap<QByteArray, MessageStatusInfo> m_servicesMessageInfo;
	QTimer m_timer;
	QFuture<MessageCounts> m_pollFuture;

protected:
	I_REF(imod::IModel, m_serviceCollectionModelCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
};


} // namespace agentgql


