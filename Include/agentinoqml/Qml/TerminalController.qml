import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtguigql 1.0

QtObject {
	id: root;

	// Identifier of the agent on which the shell is executed. Passed as the
	// "clientid" header so the server proxy routes the request to that agent.
	property string agentId: "";

	// Currently open shell session, empty when no session is open.
	property string sessionId: "";

	// Whether the remote shell process is still running.
	property bool running: false;

	// Next output sequence number to request (incremental polling cursor).
	property int nextSequence: 0;

	// Emitted with each new output chunk: stream is "STDOUT"/"STDERR"/"SYSTEM".
	signal outputReceived(string data, string stream);
	signal sessionOpened(string sessionId);
	signal sessionClosed(int exitCode);
	signal shellTypesReceived(var items);
	signal errorOccurred(string message);

	function getHeaders(){
		var headers = {};
		if (root.agentId.length > 0){
			headers["clientid"] = root.agentId;
		}
		return headers;
	}

	function addAdditionParams(inputParams){
		let additionInputParams = root.getHeaders();
		if (Object.keys(additionInputParams).length > 0){
			let additionParams = Gql.GqlObject("addition");
			for (let key in additionInputParams){
				additionParams.InsertField(key, additionInputParams[key]);
			}
			inputParams.InsertFieldObject(additionParams);
		}
	}

	function listShellTypes(){
		shellTypesModel.updateModel();
	}

	function openSession(shellType){
		openModel.open(shellType);
	}

	function sendInput(data){
		if (root.sessionId.length === 0){
			return;
		}
		sendModel.sendData(root.sessionId, data);
	}

	function closeSession(){
		if (root.sessionId.length === 0){
			return;
		}
		closeModel.closeData(root.sessionId);
	}

	function pollOutput(){
		if (root.sessionId.length === 0){
			return;
		}
		outputModel.poll(root.sessionId, root.nextSequence);
	}

	// Interval (ms) at which the UI polls the agent for new terminal output.
	property int pollIntervalMs: 400;

	// Exit code reported when a session is closed manually by the operator
	// (as opposed to the shell process finishing on its own).
	readonly property int manualCloseExitCode: -1;

	property Timer pollTimer: Timer {
		interval: root.pollIntervalMs;
		repeat: true;
		running: root.sessionId.length > 0;
		onTriggered: root.pollOutput();
	}

	property GqlModel shellTypesModel: GqlModel {
		function updateModel(){
			var query = Gql.GqlRequest("query", "ListShellTypes");
			var inputParams = Gql.GqlObject("input");
			root.addAdditionParams(inputParams);
			query.AddParam(inputParams);
			this.setGqlQuery(query.GetQuery());
		}

		onStateChanged: {
			if (this.state === "Ready" && this.containsKey("data")){
				let dataModel = this.getData("data");
				if (dataModel.containsKey("ListShellTypes")){
					let payload = dataModel.getData("ListShellTypes");
					if (payload.containsKey("items")){
						root.shellTypesReceived(payload.getData("items"));
					}
				}
			}
		}
	}

	property GqlModel openModel: GqlModel {
		function open(shellType){
			var query = Gql.GqlRequest("mutation", "OpenTerminalSession");
			var inputParams = Gql.GqlObject("input");
			inputParams.InsertField("shellType", shellType);
			root.addAdditionParams(inputParams);
			query.AddParam(inputParams);
			this.setGqlQuery(query.GetQuery());
		}

		onStateChanged: {
			if (this.state === "Ready" && this.containsKey("data")){
				let dataModel = this.getData("data");
				if (dataModel.containsKey("OpenTerminalSession")){
					let payload = dataModel.getData("OpenTerminalSession");
					if (payload.containsKey("sessionId")){
						root.sessionId = payload.getData("sessionId");
						root.nextSequence = 0;
						root.running = true;
						root.sessionOpened(root.sessionId);
					}
				}
			}
		}
	}

	property GqlModel sendModel: GqlModel {
		function sendData(sessionId, data){
			var query = Gql.GqlRequest("mutation", "SendTerminalInput");
			var inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", sessionId);
			inputParams.InsertField("data", data);
			root.addAdditionParams(inputParams);
			query.AddParam(inputParams);
			this.setGqlQuery(query.GetQuery());
		}
	}

	property GqlModel closeModel: GqlModel {
		function closeData(sessionId){
			var query = Gql.GqlRequest("mutation", "CloseTerminalSession");
			var inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", sessionId);
			root.addAdditionParams(inputParams);
			query.AddParam(inputParams);
			this.setGqlQuery(query.GetQuery());
		}

		onStateChanged: {
			if (this.state === "Ready"){
				root.running = false;
				root.sessionId = "";
				root.sessionClosed(root.manualCloseExitCode);
			}
		}
	}

	property GqlModel outputModel: GqlModel {
		function poll(sessionId, fromSequence){
			var query = Gql.GqlRequest("query", "GetTerminalOutput");
			var inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", sessionId);
			inputParams.InsertField("fromSequence", fromSequence);
			root.addAdditionParams(inputParams);
			query.AddParam(inputParams);
			this.setGqlQuery(query.GetQuery());
		}

		onStateChanged: {
			if (this.state !== "Ready" || !this.containsKey("data")){
				return;
			}
			let dataModel = this.getData("data");
			if (!dataModel.containsKey("GetTerminalOutput")){
				return;
			}
			let payload = dataModel.getData("GetTerminalOutput");

			if (payload.containsKey("chunks")){
				let chunks = payload.getData("chunks");
				let count = chunks.getItemsCount ? chunks.getItemsCount() : 0;
				for (let i = 0; i < count; ++i){
					let chunk = chunks.getItem(i);
					let stream = chunk.containsKey("stream") ? chunk.getData("stream") : "STDOUT";
					let text = chunk.containsKey("data") ? chunk.getData("data") : "";
					root.outputReceived(text, stream);
				}
			}

			if (payload.containsKey("nextSequence")){
				root.nextSequence = parseInt(payload.getData("nextSequence"));
			}

			if (payload.containsKey("finished") && payload.getData("finished") === true){
				let exitCode = payload.containsKey("exitCode") ? parseInt(payload.getData("exitCode")) : 0;
				root.running = false;
				root.sessionId = "";
				root.sessionClosed(exitCode);
			}
		}
	}
}
