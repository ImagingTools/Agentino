import QtQuick 2.0
import imtguigql 1.0
import agentinoConnectionTokensSdl 1.0

/**
	L3 transport of the token of one output connection.
	The view gets plain objects: a token {tokenId, receiverProductId, scopes, issuedAt}, a catalog
	{signInId, receiverProductId, groups, requiredPermissions, token, clientAdded} with groups as PermissionsTableView takes them.
*/
QtObject {
	id: root

	property var headersProvider: null

	signal tokenReceived(var token, bool canIssue, string cannotIssueReason)
	signal catalogReceived(var catalog)
	signal tokenChanged(var token, bool restarted, bool restartRequired)
	signal tokenRevoked()
	signal failed(string message)

	function getHeaders(){
		if (root.headersProvider && root.headersProvider.getHeaders){
			return root.headersProvider.getHeaders()
		}

		return {}
	}

	function requestToken(serviceId, connectionId){
		root.tokenInput.m_serviceId = "" + serviceId
		root.tokenInput.m_connectionId = "" + connectionId
		root.tokenRequestSender.send(root.tokenInput)
	}

	function requestCatalog(serviceId, connectionId, password){
		root.catalogInput.m_serviceId = "" + serviceId
		root.catalogInput.m_connectionId = "" + connectionId
		root.catalogInput.m_password = password
		root.catalogRequestSender.send(root.catalogInput)
	}

	function setToken(serviceId, connectionId, signInId, scopes){
		root.setInput.m_serviceId = "" + serviceId
		root.setInput.m_connectionId = "" + connectionId
		root.setInput.m_signInId = signInId
		root.setInput.m_scopes = scopes
		root.setRequestSender.send(root.setInput)
	}

	function revokeToken(serviceId, connectionId, password){
		root.revokeInput.m_serviceId = "" + serviceId
		root.revokeInput.m_connectionId = "" + connectionId
		root.revokeInput.m_password = password
		root.revokeRequestSender.send(root.revokeInput)
	}

	// The operator closed the permissions: the session at the other server ends
	function cancelSignIn(signInId){
		root.cancelInput.m_signInId = signInId
		root.cancelRequestSender.send(root.cancelInput)
	}

	function toToken(token){
		if (!token || !token.m_tokenId){
			return null
		}

		return {
			"tokenId": String(token.m_tokenId),
			"receiverProductId": token.m_receiverProductId ? String(token.m_receiverProductId) : "",
			"scopes": Array.isArray(token.m_scopes) ? token.m_scopes : [],
			"issuedAt": token.m_issuedAt ? String(token.m_issuedAt) : ""
		}
	}

	property ConnectionTokenInput tokenInput: ConnectionTokenInput {}
	property ConnectionTokenCatalogInput catalogInput: ConnectionTokenCatalogInput {}
	property SetConnectionTokenInput setInput: SetConnectionTokenInput {}
	property RevokeConnectionTokenInput revokeInput: RevokeConnectionTokenInput {}
	property CancelConnectionTokenSignInInput cancelInput: CancelConnectionTokenSignInInput {}

	// Wire command names as string literals so JQML never sends with an unresolved singleton binding
	property GqlSdlRequestSender tokenRequestSender: GqlSdlRequestSender {
		gqlCommandId: "GetConnectionToken"
		sdlObjectComp: Component {
			ConnectionTokenState {
				onFinished: {
					root.tokenReceived(root.toToken(m_token), m_canIssue === true, m_cannotIssueReason ? String(m_cannotIssueReason) : "")
				}
			}
		}

		function getHeaders(){
			return root.getHeaders()
		}

		function onError(message, type){
			root.failed(message)
		}
	}

	property GqlSdlRequestSender catalogRequestSender: GqlSdlRequestSender {
		gqlCommandId: "GetConnectionTokenCatalog"
		sdlObjectComp: Component {
			ConnectionTokenCatalog {
				onFinished: {
					root.catalogReceived({
						"signInId": String(m_signInId),
						"receiverProductId": String(m_receiverProductId),
						"groups": PermissionGroups.toFlatArray(m_groups),
						"requiredPermissions": Array.isArray(m_requiredPermissions) ? m_requiredPermissions : [],
						"token": root.toToken(m_token),
						"clientAdded": m_clientAdded === true
					})
				}
			}
		}

		function getHeaders(){
			return root.getHeaders()
		}

		function onError(message, type){
			root.failed(message)
		}
	}

	property GqlSdlRequestSender setRequestSender: GqlSdlRequestSender {
		gqlCommandId: "SetConnectionToken"
		requestType: 1
		sdlObjectComp: Component {
			SetConnectionTokenPayload {
				onFinished: {
					root.tokenChanged(root.toToken(m_token), m_restarted === true, m_restartRequired === true)
				}
			}
		}

		function getHeaders(){
			return root.getHeaders()
		}

		function onError(message, type){
			root.failed(message)
		}
	}

	property GqlSdlRequestSender revokeRequestSender: GqlSdlRequestSender {
		gqlCommandId: "RevokeConnectionToken"
		requestType: 1
		sdlObjectComp: Component {
			RevokeConnectionTokenPayload {
				onFinished: {
					if (m_success !== true){
						root.failed(qsTr("The token was not revoked"))

						return
					}

					root.tokenRevoked()
				}
			}
		}

		function getHeaders(){
			return root.getHeaders()
		}

		function onError(message, type){
			root.failed(message)
		}
	}

	property GqlSdlRequestSender cancelRequestSender: GqlSdlRequestSender {
		gqlCommandId: "CancelConnectionTokenSignIn"
		requestType: 1
		sdlObjectComp: Component {
			CancelConnectionTokenSignInPayload {}
		}

		function getHeaders(){
			return root.getHeaders()
		}
	}
}
