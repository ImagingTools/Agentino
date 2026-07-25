import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtguigql 1.0

/**
	View model of the remote terminal: owns one shell session on a single agent and
	exposes it as plain properties/signals for TerminalView.

	Every request carries the "clientid" header, which is how the server side proxy
	(agentinogql::CTerminalControllerProxyComp) decides which agent the shell belongs to.
	Output is pushed over OnTerminalOutputChanged; GetTerminalOutput is used for catch-up
	after open and after a successful re-subscribe.

	SubscriptionClient lives in TerminalView (an Item), not nested in this QtObject.
*/
QtObject {
	id: root;

	property string agentId: "";
	property string sessionId: "";
	property bool running: false;
	property int nextSequence: 0;
	// Chunks received with sequence > nextSequence (a push outran an in-flight catch-up,
	// or a resend), keyed by sequence as a string key. Held back - not applied - until
	// nextSequence reaches them, so output is always applied in order exactly once; see
	// applyOutputPayload/drainPendingChunks.
	property var pendingChunks: ({});
	property bool busy: false;
	readonly property bool requestInFlight: shellTypesRequest.state === "Loading"
									|| openRequest.state === "Loading"
									|| inputRequest.state === "Loading"
									|| rawInputRequest.state === "Loading"
									|| interruptRequest.state === "Loading"
									|| resizeRequest.state === "Loading"
									|| closeRequest.state === "Loading"
									|| outputRequest.state === "Loading";

	// True only while a command/keystroke delivery is round-tripping to the agent. Used to
	// serialise input: the GUI must not enqueue a second command until the previous
	// SendTerminalInput/SendTerminalRawInput has been acknowledged (resize/output/catch-up
	// requests are deliberately excluded - they never block the operator's typing).
	readonly property bool commandInFlight: inputRequest.state === "Loading"
									|| rawInputRequest.state === "Loading";

	// Set by TerminalView.
	property var outputSubscription: null;

	// Push channel lost while the shell session may still be open on the agent.
	// Distinct from running=false (real close).
	property bool connectionLost: false;
	property bool resubscribeAttempted: false;
	// Ignore unregistered/waiting until we have seen a healthy subscription at least once.
	property bool subscriptionEverHealthy: false;

	// Guards against double CloseTerminalSession (Close click + page destroy, etc.).
	property bool closeInFlight: false;
	property bool sessionTeardownDone: false;

	readonly property int manualCloseExitCode: -1;
	readonly property int idleCloseExitCode: -2;

	signal outputReceived(string data, string stream);
	signal sessionOpened(string sessionId);
	signal sessionClosed(int exitCode);
	signal shellTypesReceived(var items);
	signal errorOccurred(string message);
	signal connectionLostChangedSignal();

	function getHeaders(){
		let headers = {};
		if (root.agentId.length > 0){
			headers["clientid"] = root.agentId;
		}
		return headers;
	}

	function listShellTypes(){
		if (root.agentId.length === 0 || shellTypesRequest.state === "Loading"){
			return;
		}
		shellTypesRequest.send({});
	}

	function openSession(shellType){
		if (root.agentId.length === 0 || root.running || root.busy || root.closeInFlight){
			return;
		}
		root.busy = true;
		root.connectionLost = false;
		root.resubscribeAttempted = false;
		root.sessionTeardownDone = false;
		openRequest.send({"shellType": shellType});
	}

	function sendInput(data){
		if (root.sessionId.length === 0 || root.connectionLost || root.closeInFlight || root.commandInFlight){
			return;
		}
		inputRequest.send({"sessionId": root.sessionId, "data": data});
	}

	function sendRawInput(data){
		if (root.sessionId.length === 0 || root.connectionLost || root.closeInFlight || data.length === 0){
			return;
		}
		rawInputRequest.send({"sessionId": root.sessionId, "data": data});
	}

	function interruptSession(){
		if (root.sessionId.length === 0 || root.connectionLost || root.closeInFlight){
			return;
		}
		interruptRequest.send({"sessionId": root.sessionId});
	}

	// Best-effort: tells the agent's pty the GUI's current character grid so
	// full-screen/curses programs redraw correctly. Never blocks the session on failure.
	function resizeSession(columns, rows){
		if (root.sessionId.length === 0 || root.connectionLost || root.closeInFlight){
			return;
		}
		resizeRequest.send({"sessionId": root.sessionId, "columns": columns, "rows": rows});
	}

	// Request remote close; local teardown is only via forgetSession (single exit).
	function closeSession(){
		if (root.sessionId.length === 0 || root.closeInFlight || root.sessionTeardownDone){
			return;
		}
		root.closeInFlight = true;
		root.busy = true;
		closeRequest.send({"sessionId": root.sessionId});
	}

	function catchUpOutput(){
		if (root.sessionId.length === 0 || outputRequest.state === "Loading"){
			return;
		}
		outputRequest.send({"sessionId": root.sessionId, "fromSequence": root.nextSequence});
	}

	function startOutputSubscription(){
		if (root.outputSubscription && root.sessionId.length > 0){
			root.outputSubscription.registerSubscription();
		}
	}

	function stopOutputSubscription(){
		if (root.outputSubscription){
			root.outputSubscription.unRegisterSubscription();
		}
	}

	// One automatic re-subscribe + catch-up; on failure mark connectionLost.
	function handleSubscriptionHealth(state){
		if (!root.running || root.sessionId.length === 0 || root.closeInFlight || root.sessionTeardownDone){
			return;
		}

		// Healthy delivery states (initial register uses waiting/Registered).
		if (state === "Ready" || state === "Registered" || state === "Processing"){
			root.subscriptionEverHealthy = true;
			if (root.connectionLost){
				root.connectionLost = false;
				root.resubscribeAttempted = false;
				root.catchUpOutput();
			}
			return;
		}

		if (state === "waiting"){
			return;
		}

		// Do not treat the initial unregistered/empty phase as a failure.
		if (!root.subscriptionEverHealthy){
			return;
		}

		// Error / unregistered after we had a working channel.
		if (state === "Error" || state === "unregistered" || state === ""){
			if (!root.resubscribeAttempted){
				root.resubscribeAttempted = true;
				root.stopOutputSubscription();
				root.startOutputSubscription();
				// Catch-up after a short delay so register can complete.
				resubscribeCatchUpTimer.restart();
			}
			else{
				root.connectionLost = true;
				root.connectionLostChangedSignal();
			}
		}
	}

	function reconnectSubscription(){
		if (root.sessionId.length === 0 || root.closeInFlight){
			return;
		}
		root.connectionLost = false;
		root.resubscribeAttempted = false;
		root.subscriptionEverHealthy = false;
		root.stopOutputSubscription();
		root.startOutputSubscription();
		resubscribeCatchUpTimer.restart();
	}

	function fieldValue(model, key, index){
		if (!model){
			return undefined;
		}

		if (typeof model.containsKey === "function"){
			if (index === undefined){
				return model.containsKey(key) ? model.getData(key) : undefined;
			}
			return model.containsKey(key, index) ? model.getData(key, index) : undefined;
		}

		if (typeof model.ContainsKey === "function"){
			if (index === undefined){
				return model.ContainsKey(key) ? model.GetData(key) : undefined;
			}
			return model.ContainsKey(key, index) ? model.GetData(key, index) : undefined;
		}

		if (index === undefined){
			return model[key];
		}

		if (model.getItemsCount || model.GetItemsCount){
			return undefined;
		}

		let row = Array.isArray(model) ? model[index] : null;
		return row ? row[key] : undefined;
	}

	function itemsCount(model){
		if (!model){
			return 0;
		}
		if (typeof model.getItemsCount === "function"){
			return model.getItemsCount();
		}
		if (typeof model.GetItemsCount === "function"){
			return model.GetItemsCount();
		}
		if (Array.isArray(model)){
			return model.length;
		}
		if (model.count !== undefined){
			return model.count;
		}
		return 0;
	}

	function unwrapPayload(data){
		if (!data){
			return data;
		}

		let current = data;
		let commandId = "OnTerminalOutputChanged";

		if (fieldValue(current, "chunks") !== undefined || fieldValue(current, "sessionId") !== undefined){
			return current;
		}

		let nested = fieldValue(current, commandId);
		if (nested){
			return nested;
		}

		let dataWrap = fieldValue(current, "data");
		if (dataWrap){
			nested = fieldValue(dataWrap, commandId);
			if (nested){
				return nested;
			}
			if (fieldValue(dataWrap, "chunks") !== undefined){
				return dataWrap;
			}
		}

		return current;
	}

	// Applies one already-received-in-order chunk and then drains any later chunks that
	// arrived earlier (out of order) and are now contiguous with nextSequence.
	function emitChunkInOrder(sequence, stream, text){
		root.outputReceived(text, stream);
		root.nextSequence = sequence + 1;
		root.drainPendingChunks();
	}

	function drainPendingChunks(){
		let key = String(root.nextSequence);
		while (key in root.pendingChunks){
			let pending = root.pendingChunks[key];
			delete root.pendingChunks[key];
			root.outputReceived(pending.text, pending.stream);
			root.nextSequence = root.nextSequence + 1;
			key = String(root.nextSequence);
		}
	}

	function applyOutputPayload(data){
		if (!data){
			return;
		}

		let payload = unwrapPayload(data);
		if (!payload){
			return;
		}

		// upstreamHealthy:false is a synthetic, chunk-less payload the server proxy
		// manufactures when its own subscription to the agent is lost - it means the whole
		// server<->agent chain is down, not just this GUI's own WebSocket link, which a
		// merely-successful push (handled below) cannot by itself disprove.
		let upstreamHealthyValue = fieldValue(payload, "upstreamHealthy");
		if (upstreamHealthyValue === false || upstreamHealthyValue === "false"){
			if (!root.connectionLost){
				root.connectionLost = true;
				root.connectionLostChangedSignal();
			}
			return;
		}

		// Any other successful push proves the channel is alive.
		if (root.connectionLost){
			root.connectionLost = false;
			root.resubscribeAttempted = false;
		}

		let chunks = fieldValue(payload, "chunks");
		let count = itemsCount(chunks);
		for (let i = 0; i < count; ++i){
			let sequenceRaw = fieldValue(chunks, "sequence", i);
			if (sequenceRaw === undefined && Array.isArray(chunks) && chunks[i]){
				sequenceRaw = chunks[i]["sequence"];
			}

			let sequence = sequenceRaw !== undefined && sequenceRaw !== null && sequenceRaw !== ""
						? parseInt(sequenceRaw)
						: -1;

			let stream = fieldValue(chunks, "stream", i);
			let text = fieldValue(chunks, "data", i);
			if (Array.isArray(chunks) && chunks[i]){
				if (stream === undefined){
					stream = chunks[i]["stream"];
				}
				if (text === undefined){
					text = chunks[i]["data"];
				}
			}
			if (stream === undefined || stream === null || stream === ""){
				stream = "STDOUT";
			}
			if (text === undefined || text === null){
				text = "";
			}

			if (sequence < 0){
				// No usable sequence number at all (malformed/legacy payload) - apply
				// immediately rather than block the whole stream on an ordering key we
				// don't have; this is the sole exception to the ordering guarantee below.
				root.outputReceived(String(text), String(stream));
				continue;
			}

			if (sequence < root.nextSequence){
				// Already applied (duplicate delivery, e.g. overlapping catch-up + push).
				continue;
			}

			if (sequence > root.nextSequence){
				// Arrived ahead of a gap that hasn't been filled yet (e.g. a live push
				// outran an in-flight catch-up query) - hold it, do NOT advance
				// nextSequence, so the still-missing earlier chunks are not later mistaken
				// for duplicates and silently dropped once they do arrive.
				root.pendingChunks[String(sequence)] = {stream: String(stream), text: String(text)};
				continue;
			}

			root.emitChunkInOrder(sequence, String(stream), String(text));
		}

		// The server's own authoritative cursor. Only ever jumps nextSequence *forward*
		// past chunks this payload did not include - meaning they are truly gone (evicted
		// from the agent's bounded buffer before this client ever saw them), not merely
		// out of order; anything out of order was already buffered above instead of
		// relying on this jump. Chunks buffered in pendingChunks at a sequence below the
		// jumped-to value are consequently unrecoverable and are pruned so they cannot
		// wedge nextSequence's advancement forever; the client's log shows a gap, not a
		// stuck cursor.
		let reportedRaw = fieldValue(payload, "nextSequence");
		if (reportedRaw !== undefined && reportedRaw !== null && reportedRaw !== ""){
			let reported = parseInt(reportedRaw);
			if (reported > root.nextSequence){
				root.nextSequence = reported;
				for (let key in root.pendingChunks){
					if (parseInt(key) < root.nextSequence){
						delete root.pendingChunks[key];
					}
				}
				root.drainPendingChunks();
			}
		}

		let runningValue = fieldValue(payload, "running");
		if (runningValue !== undefined && runningValue !== null && runningValue !== true && runningValue !== "true" && runningValue !== 1){
			let exitRaw = fieldValue(payload, "exitCode");
			let exitCode = (exitRaw !== undefined && exitRaw !== null && exitRaw !== "") ? parseInt(exitRaw) : 0;
			root.forgetSession(exitCode);
		}
	}

	// Single local teardown path for idle / manual close / process exit / page destroy follow-up.
	function forgetSession(exitCode){
		if (root.sessionTeardownDone){
			return;
		}
		root.sessionTeardownDone = true;
		root.closeInFlight = false;
		root.stopOutputSubscription();
		root.busy = false;
		root.running = false;
		root.connectionLost = false;
		root.resubscribeAttempted = false;
		root.subscriptionEverHealthy = false;
		root.sessionId = "";
		root.nextSequence = 0;
		root.pendingChunks = ({});
		root.sessionClosed(exitCode);
	}

	property Timer resubscribeCatchUpTimer: Timer {
		interval: 400;
		repeat: false;

		onTriggered: {
			if (root.running && root.sessionId.length > 0 && !root.closeInFlight){
				root.catchUpOutput();
			}
		}
	}

	property GqlRequestSender shellTypesRequest: GqlRequestSender {
		gqlCommandId: "ListShellTypes";
		requestType: 0;

		function getHeaders(){
			return root.getHeaders();
		}

		function createQueryParams(query, params){
			let items = Gql.GqlObject("items");
			items.InsertField("type");
			items.InsertField("name");
			items.InsertField("available");
			query.AddField(items);
		}

		function onResult(data){
			if (data.containsKey("items")){
				root.shellTypesReceived(data.getData("items"));
			}
		}

		function onError(message, type){
			root.errorOccurred(message);
		}
	}

	property GqlRequestSender openRequest: GqlRequestSender {
		gqlCommandId: "OpenTerminalSession";
		requestType: 1;

		function getHeaders(){
			return root.getHeaders();
		}

		function createQueryParams(query, params){
			let inputParams = Gql.GqlObject("input");
			inputParams.InsertField("shellType", params["shellType"]);
			query.AddParam(inputParams);

			query.AddField(Gql.GqlObject("sessionId"));
			query.AddField(Gql.GqlObject("shellType"));
			query.AddField(Gql.GqlObject("started"));
		}

		function onResult(data){
			root.busy = false;

			let newSessionId = data.containsKey("sessionId") ? data.getData("sessionId") : "";
			if (newSessionId.length === 0){
				root.errorOccurred(qsTr("The agent could not start the requested shell."));
				return;
			}

			root.sessionId = newSessionId;
			root.nextSequence = 0;
			root.pendingChunks = ({});
			root.running = true;
			root.sessionTeardownDone = false;
			root.closeInFlight = false;
			root.connectionLost = false;
			root.resubscribeAttempted = false;
			root.subscriptionEverHealthy = false;
			// Register the live subscription first (its cursor starts at nextSequence, so
			// nothing produced from now on is missed), then fetch whatever was already
			// buffered before this point via one GetTerminalOutput. The subscription itself
			// does not replay history, so this catch-up is what fills the open->subscribe
			// gap; the sequence-ordered reconciliation in applyOutputPayload dedups any
			// overlap between the two.
			root.startOutputSubscription();
			root.catchUpOutput();
			root.sessionOpened(newSessionId);
		}

		function onError(message, type){
			root.busy = false;
			root.errorOccurred(message);
		}
	}

	property GqlRequestSender inputRequest: GqlRequestSender {
		gqlCommandId: "SendTerminalInput";
		requestType: 1;

		function getHeaders(){
			return root.getHeaders();
		}

		function createQueryParams(query, params){
			let inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", params["sessionId"]);
			inputParams.InsertField("data", params["data"]);
			query.AddParam(inputParams);

			query.AddField(Gql.GqlObject("accepted"));
		}

		function onResult(data){
			if (data.containsKey("accepted") && data.getData("accepted") !== true){
				root.errorOccurred(qsTr("The agent rejected the command."));
			}
		}

		function onError(message, type){
			root.errorOccurred(message);
		}
	}

	property GqlRequestSender rawInputRequest: GqlRequestSender {
		gqlCommandId: "SendTerminalRawInput";
		requestType: 1;

		function getHeaders(){
			return root.getHeaders();
		}

		function createQueryParams(query, params){
			let inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", params["sessionId"]);
			inputParams.InsertField("data", params["data"]);
			query.AddParam(inputParams);

			query.AddField(Gql.GqlObject("accepted"));
		}

		function onResult(data){
			if (data.containsKey("accepted") && data.getData("accepted") !== true){
				root.errorOccurred(qsTr("The agent rejected terminal input."));
			}
		}

		function onError(message, type){
			root.errorOccurred(message);
		}
	}

	property GqlRequestSender interruptRequest: GqlRequestSender {
		gqlCommandId: "InterruptTerminalSession";
		requestType: 1;

		function getHeaders(){
			return root.getHeaders();
		}

		function createQueryParams(query, params){
			let inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", params["sessionId"]);
			query.AddParam(inputParams);

			query.AddField(Gql.GqlObject("accepted"));
		}

		function onResult(data){
			if (data.containsKey("accepted") && data.getData("accepted") !== true){
				root.errorOccurred(qsTr("The agent could not interrupt the command."));
			}
		}

		function onError(message, type){
			root.errorOccurred(message);
		}
	}

	property GqlRequestSender resizeRequest: GqlRequestSender {
		gqlCommandId: "ResizeTerminalSession";
		requestType: 1;

		function getHeaders(){
			return root.getHeaders();
		}

		function createQueryParams(query, params){
			let inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", params["sessionId"]);
			inputParams.InsertField("columns", params["columns"]);
			inputParams.InsertField("rows", params["rows"]);
			query.AddParam(inputParams);

			query.AddField(Gql.GqlObject("resized"));
		}

		function onResult(data){
			// Best-effort - nothing to react to either way.
		}

		function onError(message, type){
			// A resize failure is not worth surfacing to the operator or the session.
		}
	}

	property GqlRequestSender closeRequest: GqlRequestSender {
		gqlCommandId: "CloseTerminalSession";
		requestType: 1;

		function getHeaders(){
			return root.getHeaders();
		}

		function createQueryParams(query, params){
			let inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", params["sessionId"]);
			query.AddParam(inputParams);

			query.AddField(Gql.GqlObject("closed"));
		}

		function onResult(data){
			root.forgetSession(root.manualCloseExitCode);
		}

		function onError(message, type){
			// Local teardown even if the agent already dropped the session.
			root.forgetSession(root.manualCloseExitCode);
			root.errorOccurred(message);
		}
	}

	property GqlRequestSender outputRequest: GqlRequestSender {
		gqlCommandId: "GetTerminalOutput";
		requestType: 0;

		function getHeaders(){
			return root.getHeaders();
		}

		function createQueryParams(query, params){
			let inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", params["sessionId"]);
			inputParams.InsertField("fromSequence", params["fromSequence"]);
			query.AddParam(inputParams);

			let chunks = Gql.GqlObject("chunks");
			chunks.InsertField("sequence");
			chunks.InsertField("stream");
			chunks.InsertField("data");
			query.AddField(chunks);

			query.AddField(Gql.GqlObject("nextSequence"));
			query.AddField(Gql.GqlObject("running"));
			query.AddField(Gql.GqlObject("exitCode"));
		}

		function onResult(data){
			root.applyOutputPayload(data);
		}

		function onError(message, type){
			root.errorOccurred(message);
		}
	}
}
