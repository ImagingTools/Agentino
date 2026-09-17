// Fixture users for the Agentino GUI suite. AgentinoServer's own auth story mirrors the rest of
// ImtCore (same AuthenticationManager/SuperuserProvider components as Lisa/ProLife) - only the
// bootstrap superuser is needed here: Agentino has no per-role/per-permission editorial workflow of
// its own worth a full user matrix (Agents/Services administration is superuser-only in practice), so
// unlike Lisa/ProLife there is deliberately no second seeded "limited" user.
const { defineUsers } = require('imtcore-gui-testkit/fixtures/defineUsers');

module.exports = defineUsers({
  users: [
    { key: 'su', title: 'Superuser', login: 'su', password: 'AgentinoGuiTest1!', seed: false, permissions: ['*'] },
  ],
  allUsersEnv: 'AGENTINO_GUI_ALL_USERS',
});
