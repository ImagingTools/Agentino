import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtguigql 1.0

/**
	L3 adapter for ServiceLogViewModel — GQL GetServiceLog.
	Scope via dataScope (QG1); getHeaders() kept as thin adapter for legacy callers.
*/
ServiceLogViewModel {
	id: root

	// Legacy alias used by existing ServiceEditor bindings
	property alias serviceLogModel: root.logModel

	function updateServiceLog(serviceId) {
		root.loading = true
		root.errorMessage = ""
		serviceLogGqlModel.updateModel(serviceId)
	}

	function getHeaders() {
		if (root.dataScope === null)
			return {}
		let headers = {}
		if (root.dataScope.agentId && root.dataScope.agentId.length > 0)
			headers.clientid = root.dataScope.agentId
		if (root.dataScope.serviceId && root.dataScope.serviceId.length > 0)
			headers.serviceid = root.dataScope.serviceId
		return headers
	}

	onRefresh: updateServiceLog(serviceId)

	property GqlModel serviceLogGqlModel: GqlModel {
		function updateModel(serviceId) {
			var query = Gql.GqlRequest("query", "GetServiceLog")

			var inputParams = Gql.GqlObject("input")
			inputParams.InsertField("id", serviceId)

			let additionInputParams = root.getHeaders()
			if (Object.keys(additionInputParams).length > 0) {
				let additionParams = Gql.GqlObject("addition")
				for (let key in additionInputParams) {
					additionParams.InsertField(key, additionInputParams[key])
				}
				inputParams.InsertFieldObject(additionParams)
			}
			query.AddParam(inputParams)

			var gqlData = query.GetQuery()
			this.setGqlQuery(gqlData)
		}

		onStateChanged: {
			if (this.state === "Ready") {
				root.loading = false
				if (root.serviceLogGqlModel.containsKey("errors")) {
					root.errorMessage = "GetServiceLog failed"
					return
				}
				if (root.serviceLogGqlModel.containsKey("data")) {
					var dataModelLocal = root.serviceLogGqlModel.getData("data")
					if (dataModelLocal.containsKey("GetServiceLog")) {
						dataModelLocal = dataModelLocal.getData("GetServiceLog")
						root.setLogModel(dataModelLocal)
					}
				}
			}
		}
	}
}
