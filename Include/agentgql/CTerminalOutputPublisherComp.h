// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QMap>
#include <QtCore/QMutex>

// ACF includes
#include <imod/TSingleModelObserverBase.h>

// ImtCore includes
#include <imtservergql/CGqlPublisherCompBase.h>

// Agentino includes
#include <agentinodata/ITerminalController.h>


namespace agentgql
{


/**
	Publishes \c OnTerminalOutputChanged when a terminal session produces new output.

	Observes \ref agentinodata::ITerminalController for
	\ref agentinodata::ITerminalController::CN_TERMINAL_OUTPUT_CHANGED, filters
	registered GraphQL subscriptions by \c sessionId, and pushes only the chunks each
	subscriber has not seen yet (per-subscription cursor). Same pattern as
	\ref agentinogql::CServiceSubscriberControllerComp for status, but with parameter
	filtering that status/log subscriptions do not need.
*/
class CTerminalOutputPublisherComp:
			public imtservergql::CGqlPublisherCompBase,
			public imod::TSingleModelObserverBase<istd::IChangeable>
{
public:
	typedef imtservergql::CGqlPublisherCompBase BaseClass;

	I_BEGIN_COMPONENT(CTerminalOutputPublisherComp);
		I_ASSIGN(
					m_terminalControllerCompPtr,
					"TerminalController",
					"Terminal session manager that owns shell processes and buffers output",
					true,
					"TerminalSessionManager");
		I_ASSIGN_TO(m_modelCompPtr, m_terminalControllerCompPtr, true);
	I_END_COMPONENT;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

	// reimplemented (imtgql::IGqlSubscriberController)
	virtual bool RegisterSubscription(
				const QByteArray& subscriptionId,
				const imtgql::CGqlRequest& gqlRequest,
				const imtrest::IRequest& networkRequest,
				QString& errorMessage) override;
	virtual bool UnregisterSubscription(const QByteArray& subscriptionId) override;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

private:
	static QByteArray ExtractSessionId(const imtgql::CGqlRequest& gqlRequest);
	static QString StreamToWire(agentinodata::ITerminalController::StreamType stream);
	void PublishForSession(const QByteArray& sessionId);

private:
	I_REF(agentinodata::ITerminalController, m_terminalControllerCompPtr);
	I_REF(imod::IModel, m_modelCompPtr);

	// Per GraphQL subscription: next sequence number that subscriber has not received yet.
	mutable QMutex m_cursorMutex;
	QMap<QByteArray, qint64> m_subscriptionCursors;
};


} // namespace agentgql
