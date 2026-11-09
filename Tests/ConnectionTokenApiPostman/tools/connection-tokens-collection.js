// Builds Connection_Tokens.postman_collection.json: run "node tools/connection-tokens-collection.js" after editing the cases.
// Every expectation was measured on the server; refusals pin their reason.
const fs = require('fs');
const path = require('path');

const collectionPath = path.join(__dirname, '..', 'Connection_Tokens.postman_collection.json');

// A request to the Agentino server; options.agent adds the agent the command is about
function item(name, query, options = {}){
	const headers = [];
	if (options.auth){
		headers.push({ key: 'x-authentication-token', value: options.auth });
	}
	if (options.agent !== false){
		headers.push({ key: 'clientid', value: '{{client_id}}' });
	}

	return {
		name,
		event: [{ listen: 'test', script: { type: 'text/javascript', exec: options.test } }],
		request: {
			method: 'POST',
			header: headers,
			body: { mode: 'graphql', graphql: { query, variables: '' } },
			url: { raw: '{{base_url}}', host: ['{{base_url}}'] }
		},
		response: []
	};
}

function refused(pattern){
	return [
		'const json = pm.response.json();',
		"const messages = (json.errors || []).map(function (e) { return e.message || ''; }).join(' | ');",
		`pm.test('refused: ${pattern.replace(/\\/g, '').replace(/'/g, '')}', function () { pm.expect(messages, JSON.stringify(json)).to.match(${pattern}); });`,
		"pm.test('no data', function () { const data = json.data || {}; Object.keys(data).forEach(function (k) { pm.expect(data[k], k).to.eql(null); }); });"
	];
}

function accepted(command, checks){
	return [
		'const json = pm.response.json();',
		`const payload = json.data && json.data.${command};`,
		`pm.test('${command} answered', function () { pm.expect(json.errors, JSON.stringify(json.errors)).to.eql(undefined); pm.expect(payload, JSON.stringify(json)).to.be.an('object'); });`,
		...(checks || [])
	];
}

// The commands of the connection tokens; the service and the connection do not have to exist for the access checks
const commands = {
	GetConnectionToken: 'query GetConnectionToken { GetConnectionToken(input: { serviceId: "ct-service", connectionId: "ct-connection" }) { token { tokenId } canIssue cannotIssueReason } }',
	GetConnectionTokenCatalog: 'query GetConnectionTokenCatalog { GetConnectionTokenCatalog(input: { serviceId: "ct-service", connectionId: "ct-connection", password: "any" }) { signInId } }',
	SetConnectionToken: 'mutation SetConnectionToken { SetConnectionToken(input: { serviceId: "ct-service", connectionId: "ct-connection", signInId: "ct-sign-in", scopes: [] }) { restartRequired } }',
	RevokeConnectionToken: 'mutation RevokeConnectionToken { RevokeConnectionToken(input: { serviceId: "ct-service", connectionId: "ct-connection", password: "any" }) { success } }',
	CancelConnectionTokenSignIn: 'mutation CancelConnectionTokenSignIn { CancelConnectionTokenSignIn(input: { signInId: "ct-sign-in" }) { success } }'
};
const operatorOnly = refused('/Connection tokens are managed by a superuser session of Agentino/');
const spentSignIn = refused('/The sign-in has expired or has been used, sign in again/');

const bootstrap = {
	name: '00 Bootstrap',
	item: [
		item('su signs in', 'query Authorization { Authorization(input: { login: "{{su_login}}", password: "{{su_password}}" }) { token } }', {
			agent: false,
			test: accepted('Authorization', ["pm.test('su token', function () { pm.expect(payload.token).to.be.a('string').and.not.empty; });", "pm.environment.set('su_token', payload.token);"])
		}),
		Object.assign(item('su adds an ordinary user', 'mutation UserAdd { UserAdd(input: { id: "{{ct_user_id}}", typeId: "User", productId: "Agentino", item: { id: "{{ct_user_id}}", productId: "Agentino", name: "Connection token probe", username: "{{ct_user_login}}", password: "CtProbe_2026x", email: "{{ct_user_login}}@test.local", groups: [], roles: [], permissions: [], systemInfos: [] } }) { id } }', {
			auth: '{{su_token}}', agent: false,
			test: accepted('UserAdd', ["pm.test('added', function () { pm.expect(payload.id).to.eql(pm.environment.get('ct_user_id')); });"])
		}), {
			event: [
				{ listen: 'prerequest', script: { type: 'text/javascript', exec: ["pm.environment.set('ct_user_id', pm.variables.replaceIn('{{$guid}}'));", "pm.environment.set('ct_user_login', 'ct.probe.' + Date.now());"] } },
				{ listen: 'test', script: { type: 'text/javascript', exec: accepted('UserAdd', ["pm.test('added', function () { pm.expect(payload.id).to.eql(pm.environment.get('ct_user_id')); });"]) } }
			]
		}),
		item('the ordinary user signs in', 'query Authorization { Authorization(input: { login: "{{ct_user_login}}", password: "CtProbe_2026x" }) { token } }', {
			agent: false,
			test: accepted('Authorization', ["pm.test('user token', function () { pm.expect(payload.token).to.be.a('string').and.not.empty; });", "pm.environment.set('user_token', payload.token);"])
		})
	]
};

const access = { name: '01 Who manages connection tokens', item: [] };
for (const [command, query] of Object.entries(commands)){
	access.item.push(item(`${command}: anonymous is refused`, query, { test: operatorOnly }));
	access.item.push(item(`${command}: an ordinary user is refused`, query, { auth: '{{user_token}}', test: operatorOnly }));
}

const signIns = {
	name: '02 Sign-ins at the receiving server',
	item: [
		item('su reads the token of a connection', commands.GetConnectionToken, {
			auth: '{{su_token}}',
			test: accepted('GetConnectionToken', ["pm.test('no token held', function () { pm.expect(payload.token || null).to.eql(null); });", "pm.test('no token can be issued for a service of no agent, and why', function () { pm.expect(payload.canIssue).to.eql(false); pm.expect(payload.cannotIssueReason).to.match(/The service 'ct-service' of agent '[^']+' is unknown/); });"])
		}),
		item('a token is not changed without a sign-in of this server', commands.SetConnectionToken, { auth: '{{su_token}}', test: spentSignIn }),
		item('an unknown sign-in cannot be cancelled', commands.CancelConnectionTokenSignIn, { auth: '{{su_token}}', test: spentSignIn }),
		item('a service of no agent signs in nowhere', commands.GetConnectionTokenCatalog, {
			auth: '{{su_token}}',
			test: refused("/The service 'ct-service' of agent '[^']+' is unknown/")
		}),
		item('a token of a service of no agent is not revoked', commands.RevokeConnectionToken, {
			auth: '{{su_token}}',
			test: refused("/The service 'ct-service' of agent '[^']+' is unknown/")
		})
	]
};

const collection = {
	info: {
		name: 'Agentino connection tokens',
		description: 'Who manages the tokens of the output connections and how a sign-in at the receiving server is used. Run by Run-CiTests.ps1.',
		schema: 'https://schema.getpostman.com/json/collection/v2.1.0/collection.json'
	},
	item: [bootstrap, access, signIns]
};

fs.writeFileSync(collectionPath, JSON.stringify(collection, null, 4).replace(/\n/g, '\r\n') + '\r\n');
console.log(`${bootstrap.item.length + access.item.length + signIns.item.length} requests`);
