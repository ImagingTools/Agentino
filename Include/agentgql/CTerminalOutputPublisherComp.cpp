// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CTerminalOutputPublisherComp.h>


// std includes
#include <limits>

// Qt includes
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>


namespace agentgql
{


// protected methods

// reimplemented (icomp::CComponentBase)

void CTerminalOutputPublisherComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_modelCompPtr.IsValid()){
		m_modelCompPtr->AttachObserver(this);
	}
	else{
		SendErrorMessage(
					0,
					"Attribute 'TerminalController' is not set — terminal output publish is disabled",
					"CTerminalOutputPublisherComp");
	}
}


void CTerminalOutputPublisherComp::OnComponentDestroyed()
{
	if (m_modelCompPtr.IsValid()){
		m_modelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


// reimplemented (imtgql::IGqlSubscriberController)

bool CTerminalOutputPublisherComp::RegisterSubscription(
			const QByteArray& subscriptionId,
			const imtgql::CGqlRequest& gqlRequest,
			const imtrest::IRequest& networkRequest,
			QString& errorMessage)
{
	const bool retVal = BaseClass::RegisterSubscription(subscriptionId, gqlRequest, networkRequest, errorMessage);
	if (!retVal){
		return false;
	}

	// Start the cursor at the current end of the buffer so only post-subscribe output
	// is pushed. The GUI does one GetTerminalOutput catch-up for history.
	qint64 cursor = 0;
	if (m_terminalControllerCompPtr.IsValid()){
		const QByteArray sessionId = ExtractSessionId(gqlRequest);
		if (!sessionId.isEmpty()){
			qint64 nextSequence = 0;
			bool running = false;
			int exitCode = -1;
			m_terminalControllerCompPtr->GetOutput(
						sessionId,
						std::numeric_limits<qint64>::max(),
						nextSequence,
						running,
						exitCode);
			cursor = nextSequence;
		}
	}

	QMutexLocker locker(&m_cursorMutex);
	m_subscriptionCursors.insert(subscriptionId, cursor);

	return true;
}


bool CTerminalOutputPublisherComp::UnregisterSubscription(const QByteArray& subscriptionId)
{
	{
		QMutexLocker locker(&m_cursorMutex);
		m_subscriptionCursors.remove(subscriptionId);
	}

	return BaseClass::UnregisterSubscription(subscriptionId);
}


// reimplemented (imod::CSingleModelObserverBase)

void CTerminalOutputPublisherComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!changeSet.GetChangeInfo(agentinodata::ITerminalController::CN_TERMINAL_OUTPUT_CHANGED).isValid()){
		return;
	}

	const QByteArray sessionId =
				changeSet.GetChangeInfo(agentinodata::ITerminalController::CN_TERMINAL_OUTPUT_CHANGED).toByteArray();
	if (sessionId.isEmpty()){
		return;
	}

	PublishForSession(sessionId);
}


// private methods

QByteArray CTerminalOutputPublisherComp::ExtractSessionId(const imtgql::CGqlRequest& gqlRequest)
{
	const imtgql::CGqlParamObject* inputPtr = gqlRequest.GetParamObject("input");
	if (inputPtr == nullptr){
		return QByteArray();
	}

	return inputPtr->GetParamArgumentValue("sessionId").toByteArray();
}


QString CTerminalOutputPublisherComp::StreamToWire(agentinodata::ITerminalController::StreamType stream)
{
	switch (stream){
	case agentinodata::ITerminalController::STREAM_STDERR:
		return QStringLiteral("STDERR");
	case agentinodata::ITerminalController::STREAM_SYSTEM:
		return QStringLiteral("SYSTEM");
	case agentinodata::ITerminalController::STREAM_STDOUT:
	default:
		return QStringLiteral("STDOUT");
	}
}


void CTerminalOutputPublisherComp::PublishForSession(const QByteArray& sessionId)
{
	if (!m_terminalControllerCompPtr.IsValid()){
		return;
	}

	static const QByteArray s_commandId = QByteArrayLiteral("OnTerminalOutputChanged");

	// Hold the publisher mutex for the whole push (see CGqlPublisherCompBase::PublishDataFiltered).
	QMutexLocker locker(&m_mutex);

	for (const RequestNetworks& entry : m_registeredSubscribers){
		if (entry.gqlRequest.GetCommandId() != s_commandId){
			continue;
		}

		if (ExtractSessionId(entry.gqlRequest) != sessionId){
			continue;
		}

		for (auto it = entry.networkRequests.constBegin(); it != entry.networkRequests.constEnd(); ++it){
			const QByteArray& subscriptionId = it.key();
			const imtrest::IRequest* networkRequestPtr = it.value();
			if (networkRequestPtr == nullptr){
				continue;
			}

			qint64 fromSequence = 0;
			{
				QMutexLocker cursorLocker(&m_cursorMutex);
				fromSequence = m_subscriptionCursors.value(subscriptionId, 0);
			}

			qint64 nextSequence = fromSequence;
			bool running = false;
			int exitCode = -1;
			const QList<agentinodata::ITerminalController::OutputChunk> chunks =
						m_terminalControllerCompPtr->GetOutput(
									sessionId, fromSequence, nextSequence, running, exitCode);

			if (chunks.isEmpty() && running){
				// Nothing new for this subscriber yet (another subscriber may have been behind).
				continue;
			}

			QJsonArray chunksArray;
			for (const agentinodata::ITerminalController::OutputChunk& chunk : chunks){
				QJsonObject chunkObject;
				chunkObject.insert(QStringLiteral("sequence"), static_cast<qint64>(chunk.sequence));
				chunkObject.insert(QStringLiteral("stream"), StreamToWire(chunk.stream));
				chunkObject.insert(QStringLiteral("data"), chunk.data);
				chunksArray.append(chunkObject);
			}

			QJsonObject payload;
			payload.insert(QStringLiteral("sessionId"), QString::fromUtf8(sessionId));
			payload.insert(QStringLiteral("chunks"), chunksArray);
			payload.insert(QStringLiteral("nextSequence"), static_cast<qint64>(nextSequence));
			payload.insert(QStringLiteral("running"), running);
			payload.insert(QStringLiteral("exitCode"), exitCode);

			const QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
			if (!PushDataToSubscriber(subscriptionId, s_commandId, body, *networkRequestPtr)){
				SendErrorMessage(
							0,
							QString("Unable to push terminal output for session '%1' (subscription '%2')")
										.arg(QString::fromUtf8(sessionId), QString::fromUtf8(subscriptionId)),
							"CTerminalOutputPublisherComp");
				continue;
			}

			QMutexLocker cursorLocker(&m_cursorMutex);
			m_subscriptionCursors.insert(subscriptionId, nextSequence);
		}
	}
}


} // namespace agentgql
