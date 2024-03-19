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

		int loginStatus = m_loginStatusProviderCompPtr->GetLoginStatus();
		if (loginStatus == 0){
			status = "Disconnected";
		}
		else if (loginStatus == imtauth::ILoginStatusProvider::LSF_LOGGED_IN){
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
	if (changeSet.Contains(0)){
		status = "Disconnected";
	}
	else if (changeSet.Contains(imtauth::ILoginStatusProvider::LSF_LOGGED_IN)){
		status = "Connected";
	}

	if (changeSet.Contains(0) || changeSet.Contains(imtauth::ILoginStatusProvider::LSF_LOGGED_IN)){
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


