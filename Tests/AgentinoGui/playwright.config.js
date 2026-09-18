// Thin wrapper around the shared testkit config builder - see imtcore-gui-testkit's own
// playwrightConfig/createConfig.js doc comment for the full list of policy this applies.
//
// Scope: this config targets AgentinoServer's own web UI (Agents / Topology / Administration pages),
// which is where the su fixture user logs in during global-setup. AgentinoAgent.exe hosts a SEPARATE,
// much smaller web UI of its own (just an "AgentSettings" connect screen, its own Services view and a
// remote Terminal) on a different port (7222 by default) with its own independent auth. Rather than
// invent a two-baseURL config, the handful of Agent-app specs navigate there with an explicit absolute
// URL (see tests/agent-settings.test.js) instead of relying on this config's baseURL/session fixtures.
const { createGuiConfig } = require('imtcore-gui-testkit/playwrightConfig/createConfig');
const users = require('./fixtures/users');

module.exports = createGuiConfig({
  rootDir: __dirname,
  baseUrl: process.env.AGENTINO_GUI_BASE_URL || 'http://localhost:17111',
  users,
  globalSetup: require.resolve('./global-setup'),
  mutatingUserKeys: ['su'],
  workers: 1,
});
