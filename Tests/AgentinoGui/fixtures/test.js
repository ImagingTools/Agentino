// Extended Playwright test with Agentino-aware fixtures.
//
//   test('...', async ({ page, user, gui }) => { ... })
//
// The fixture/newUserPage/forEachUser machinery is generic (imtcore-gui-testkit); this file only wires
// it to Agentino's own fixtures/users.js.

const path = require('path');
const { createGuiTest } = require('imtcore-gui-testkit/fixtures/createTest');
const users = require('./users');

module.exports = createGuiTest(users, { rootDir: path.resolve(__dirname, '..') });
