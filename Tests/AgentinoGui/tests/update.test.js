// Update page (UpdatePage.qml) - the service types installed on the agents with the number of agents
// using each of them, filtered by the operating system of the agents. Which services and operating
// systems exist depends on the agents running against this instance, so the rows are checked
// structurally (from the GraphQL responses), not against a screenshot.

const { test, expect, gui } = require('../fixtures/test');
const { UpdatePage } = require('../pages');
const { refuse } = require('../fixtures/refuse');

// ServiceUpdatesList items as {name: agentCount}, after checking the invariants of every row.
function agentCounts(items) {
  const names = items.map((item) => item.name);
  expect(new Set(names).size, 'every service type is listed once').toBe(names.length);
  const counts = {};
  for (const item of items) {
    expect(item.name, 'a row names its service type').toBeTruthy();
    expect(item.agentCount, `"${item.name}" is used by at least one agent`).toBeGreaterThan(0);
    expect(item.latestVersion || '', 'the latest version is not provided yet').toBe('');
    counts[item.name] = item.agentCount;
  }
  return counts;
}

test.describe('Update', () => {
  test.beforeEach(async ({ page }) => {
    await new UpdatePage(page).reload();
  });

  test('lists the service types with their agent counts', async ({ page }) => {
    const update = new UpdatePage(page);
    if (!(await update.isAvailable())) refuse('the Update page is not in the menu');

    const servicesSpy = gui.graphql.spyOn(page, 'ServiceUpdatesList');
    const osSpy = gui.graphql.spyOn(page, 'AgentOperatingSystemsList');
    await update.open();
    const services = await servicesSpy.wait();
    expect(services.errors, 'ServiceUpdatesList must succeed').toBeFalsy();
    const operatingSystems = await osSpy.wait();
    expect(operatingSystems.errors, 'AgentOperatingSystemsList must succeed').toBeFalsy();
    await update.expectOpen();

    agentCounts(services.data.ServiceUpdatesList.items);
    const names = operatingSystems.data.AgentOperatingSystemsList.names || [];
    expect(new Set(names).size, 'every operating system family is listed once').toBe(names.length);
    const families = ['Windows', 'Linux', 'macOS', 'Other'];
    expect(names.every((name) => families.includes(name)), `operating systems are grouped into families: ${names}`).toBe(true);
    expect(names, 'families are listed as Windows, Linux, macOS, Other').toEqual(families.filter((f) => names.includes(f)));
  });

  test('selecting an operating system filters the table', async ({ page }) => {
    const update = new UpdatePage(page);
    if (!(await update.isAvailable())) refuse('the Update page is not in the menu');

    const allSpy = gui.graphql.spyOn(page, 'ServiceUpdatesList');
    const osSpy = gui.graphql.spyOn(page, 'AgentOperatingSystemsList');
    await update.open();
    const all = agentCounts((await allSpy.wait()).data.ServiceUpdatesList.items);
    const names = (await osSpy.wait()).data.AgentOperatingSystemsList.names || [];
    test.skip(names.length === 0, 'no agent of this instance reported its operating system');

    const filteredSpy = gui.graphql.spyOn(page, 'ServiceUpdatesList');
    await update.selectFirstOperatingSystem();
    const filtered = await filteredSpy.wait();
    expect(filtered.errors, 'the filtered ServiceUpdatesList must succeed').toBeFalsy();
    const counts = agentCounts(filtered.data.ServiceUpdatesList.items);
    for (const [name, count] of Object.entries(counts)) {
      expect(all[name], `"${name}" is in the unfiltered list as well`).toBeDefined();
      expect(count, `"${name}" is not used by more agents of one operating system than of all`).toBeLessThanOrEqual(all[name]);
    }

    const backSpy = gui.graphql.spyOn(page, 'ServiceUpdatesList');
    await update.selectAllOperatingSystems();
    expect(agentCounts((await backSpy.wait()).data.ServiceUpdatesList.items), '"All" shows every agent again').toEqual(all);
  });
});
