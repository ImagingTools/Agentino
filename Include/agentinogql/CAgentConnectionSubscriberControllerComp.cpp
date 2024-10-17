#include <agentinogql/CAgentConnectionSubscriberControllerComp.h>


// ImtCore includes
#include<imtrest/IProtocolEngine.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtgql::CGqlSubscriberControllerCompBase)

bool CAgentConnectionSubscriberControllerComp::RegisterSubscription(
			const QByteArray& subscriptionId,
			const imtgql::CGqlRequest& gqlRequest,
			const imtrest::IRequest& networkRequest,
			QString& errorMessage)
{
	bool result = BaseClass::RegisterSubscription(subscriptionId, gqlRequest, networkRequest, errorMessage);
	if (result){
		QByteArray status;

		imtcom::IConnectionStatusProvider::ConnectionStatus loginStatus
					= m_loginStatusProviderCompPtr->GetConnectionStatus();
		if (loginStatus == imtcom::IConnectionStatusProvider::CS_DISCONNECTED){
			status = "Disconnected";
		}
		else if (loginStatus == imtcom::IConnectionStatusProvider::CS_CONNECTED){
			status = "Connected";
		}

		QString data = QString("{\"status\": \"%1\"}").arg(qPrintable(status));

		SetData(subscriptionId, "OnAgentConnectionChanged", data.toUtf8(), networkRequest);
	}

	return result;
}


// reimplemented (imod::CSingleModelObserverBase)

void CAgentConnectionSubscriberControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_requestManagerCompPtr.IsValid()){
		return;
	}

	QByteArray status;
	if (changeSet.Contains(imtcom::IConnectionStatusProvider::CS_DISCONNECTED)){
		status = "Disconnected";
	}
	else if (changeSet.Contains(imtcom::IConnectionStatusProvider::CS_CONNECTED)){
		status = "Connected";
	}

	if (changeSet.Contains(0) || changeSet.Contains(imtcom::IConnectionStatusProvider::CS_CONNECTED)){
		QString data = QString("{\"status\": \"%1\"}").arg(qPrintable(status));

		SetAllSubscriptions("OnAgentConnectionChanged", data.toUtf8());
	}
}


// reimplemented (icomp::CComponentBase)

void CAgentConnectionSubscriberControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_loginStatusModelCompPtr.IsValid()) {
		m_loginStatusModelCompPtr->AttachObserver(this);
	}
}


void CAgentConnectionSubscriberControllerComp::OnComponentDestroyed()
{
	if (m_loginStatusModelCompPtr.IsValid()){
		m_loginStatusModelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


} // namespace agentinogql


