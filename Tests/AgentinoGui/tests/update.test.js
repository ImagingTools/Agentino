// Update page (UpdatePage.qml) - the service types installed on the agents with the number of agents
// using each of them. Which services exist depends on the agents running against this instance, so the
// rows are checked structurally (from the ServiceUpdatesList response), not against a screenshot.

const { test, expect, gui } = require('../fixtures/test');
const { UpdatePage } = require('../pages');
const { refuse } = require('../fixtures/refuse');

test.describe('Update', () => {
  test.beforeEach(async ({ page }) => {
    await new UpdatePage(page).reload();
  });

  test('lists the service types with their agent counts', async ({ page }) => {
    const update = new UpdatePage(page);
    if (!(await update.isAvailable())) refuse('the Update page is not in the menu');

    const spy = gui.graphql.spyOn(page, 'ServiceUpdatesList');
    await update.open();
    const { errors, data } = await spy.wait();
    expect(errors, 'ServiceUpdatesList must succeed').toBeFalsy();
    await update.expectOpen();

    const items = data.ServiceUpdatesList.items;
    const names = items.map((item) => item.name);
    expect(new Set(names).size, 'every service type is listed once').toBe(names.length);
    for (const item of items) {
      expect(item.name, 'a row names its service type').toBeTruthy();
      expect(item.agentCount, `"${item.name}" is used by at least one agent`).toBeGreaterThan(0);
      expect(item.latestVersion || '', 'the latest version is not provided yet').toBe('');
    }
  });
});
