import QtQuick 2.0
import Acf 1.0
import imtguigql 1.0
import agentinoServicesSdl 1.0

/**
	L3 transport for ServiceEditorWrap (QG2).
	Holds GQL senders / subscription; editor view binds via signals only.
*/
QtObject {
	id: root

	property var headersProvider: null

	signal settingsLoaded(string content, bool exists, string path)
	signal settingsSaved(string content, bool exists, string path)
	signal pluginLoaded(var pluginInfo)
	signal pluginLoadFailed()
	signal collectionChangedForService(string serviceId)
	signal availableConnectionsLoaded(var payload)
	signal outputConnectionSet(string connectionId, bool successful, var connectionParam)

	function getHeaders() {
		if (headersProvider && headersProvider.getHeaders)
			return headersProvider.getHeaders()
		return {}
	}

	function loadSettings(serviceId) {
		getServiceSettingsInput.m_serviceId = serviceId
		getServiceSettingsRequestSender.send(getServiceSettingsInput)
	}

	function saveSettings(serviceId, content) {
		updateServiceSettingsInput.m_serviceId = "" + serviceId
		updateServiceSettingsInput.m_content = "" + content
		updateServiceSettingsRequestSender.send(updateServiceSettingsInput)
	}

	function loadPlugin(servicePath) {
		loadPluginInput.m_servicePath = servicePath
		loadPluginRequestSender.send(loadPluginInput)
	}

	/**
		Candidate producers for the given output slots. Asked for only when the editor
		actually needs the pick-list, so ordinary reads of a service stay cheap.
	*/
	function loadAvailableConnections(connectionUsageIds) {
		availableConnectionsInput.m_connectionUsageIds = connectionUsageIds
		availableConnectionsRequestSender.send(availableConnectionsInput)
	}

	/**
		Apply immediately - independent of the rest of ServiceData, so picking a producer
		for one Output Connection does not require saving the whole service record.
		dependantConnectionId === "" clears the selection.
	*/
	function setOutputConnection(serviceId, connectionId, dependantConnectionId) {
		setOutputConnectionInput.m_serviceId = "" + serviceId
		setOutputConnectionInput.m_connectionId = "" + connectionId
		setOutputConnectionInput.m_dependantConnectionId = "" + dependantConnectionId
		setOutputConnectionRequestSender.connectionId = "" + connectionId
		setOutputConnectionRequestSender.send(setOutputConnectionInput)
	}

	property ServiceInput getServiceSettingsInput: ServiceInput {}
	property ServiceSettingsInput updateServiceSettingsInput: ServiceSettingsInput {}
	property LoadPluginInput loadPluginInput: LoadPluginInput {}
	property AvailableConnectionsInput availableConnectionsInput: AvailableConnectionsInput {}
	property SetOutputConnectionInput setOutputConnectionInput: SetOutputConnectionInput {}

	// Wire command names (match AgentinoServicesSdlCommandIds) as string literals so
	// JQML never sends with an unresolved singleton binding.
	property GqlSdlRequestSender availableConnectionsRequestSender: GqlSdlRequestSender {
		gqlCommandId: "AvailableConnections"
		sdlObjectComp: Component {
			AvailableConnectionsPayload {
				onFinished: root.availableConnectionsLoaded(this)
			}
		}
		function getHeaders() { return root.getHeaders() }
	}

	property GqlSdlRequestSender setOutputConnectionRequestSender: GqlSdlRequestSender {
		property string connectionId: ""
		requestType: 1
		gqlCommandId: "SetOutputConnection"
		sdlObjectComp: Component {
			SetOutputConnectionResponse {
				onFinished: {
					let ok = m_succesful === true
					root.outputConnectionSet(root.setOutputConnectionRequestSender.connectionId, ok, ok ? m_connectionParam : null)
				}
			}
		}
		function getHeaders() { return root.getHeaders() }
	}

	property GqlSdlRequestSender getServiceSettingsRequestSender: GqlSdlRequestSender {
		gqlCommandId: "GetServiceSettings"
		sdlObjectComp: Component {
			ServiceSettingsPayload {
				onFinished: root.settingsLoaded(m_content, m_exists, m_path)
			}
		}
		function getHeaders() { return root.getHeaders() }
	}

	property GqlSdlRequestSender updateServiceSettingsRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: "UpdateServiceSettings"
		sdlObjectComp: Component {
			ServiceSettingsPayload {
				onFinished: root.settingsSaved(m_content, m_exists, m_path)
			}
		}
		function getHeaders() { return root.getHeaders() }
	}

	property GqlSdlRequestSender loadPluginRequestSender: GqlSdlRequestSender {
		gqlCommandId: "LoadPlugin"
		requestType: 1
		onFinished: {
			if (status === -1)
				root.pluginLoadFailed()
		}
		sdlObjectComp: Component {
			PluginInfo {
				onFinished: root.pluginLoaded(this)
			}
		}
		function getHeaders() { return root.getHeaders() }
	}

	property SubscriptionClient subscriptionClient: SubscriptionClient {
		gqlCommandId: "OnServicesCollectionChanged"
		function getHeaders() { return root.getHeaders() }
		onMessageReceived: {
			if (data.containsKey("serviceid"))
				root.collectionChangedForService(data.getData("serviceid"))
		}
	}
}
