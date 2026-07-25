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

	\note RegisterSubscription honours the client's \c fromSequence as the initial cursor
	but does NOT itself push a catch-up: every new chunk is delivered by \ref OnUpdate
	(the change-notification from the session manager), and the GUI issues one
	\c GetTerminalOutput after opening to fetch whatever was produced before this
	subscription registered. Pushing from inside RegisterSubscription's own call frame -
	before the GraphQL/WebSocket layer has finished acknowledging the subscribe - is a
	protocol-ordering hazard, so it is deliberately avoided here.
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
		// Same component as TerminalController, but resolved as imod::IModel so this
		// publisher can observe CN_TERMINAL_OUTPUT_CHANGED. A direct I_ASSIGN (resolved
		// through the framework's IChangeable->IModel bridge, exactly like
		// CServiceSubscriberControllerComp's "Model") is used rather than I_ASSIGN_TO from
		// m_terminalControllerCompPtr: ITerminalController derives from istd::IChangeable,
		// not imod::IModel, so following that ref yields no IModel and OnUpdate never fires.
		I_ASSIGN(
					m_modelCompPtr,
					"Model",
					"Observed model (the terminal session manager) emitting CN_TERMINAL_OUTPUT_CHANGED",
					true,
					"TerminalSessionManager");
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
	static qint64 ExtractFromSequence(const imtgql::CGqlRequest& gqlRequest);
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
