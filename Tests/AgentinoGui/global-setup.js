// Thin wrapper around the shared testkit global-setup factory - see imtcore-gui-testkit's own
// globalSetup/createGlobalSetup.js doc comment for what this actually does (logs every active fixture
// user in through the UI once and saves its storageState; nothing gets created here).
const { createGlobalSetup } = require('imtcore-gui-testkit/globalSetup/createGlobalSetup');
const { activeUsers, authFile } = require('./fixtures/users');

module.exports = createGlobalSetup({
  activeUsers,
  authFile,
  rootDir: __dirname,
  baseUrl: process.env.AGENTINO_GUI_BASE_URL || 'http://localhost:7111',
  reuseExistingAuth: process.env.AGENTINO_GUI_REUSE_AUTH === '1',
});
