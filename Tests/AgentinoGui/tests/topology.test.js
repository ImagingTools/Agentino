// Topology page (TopologyPage.qml) - a live diagram of every agent's services. See TopologyPage's
// class doc for why node-level interaction is not covered: SchemeView renders diagram nodes without
// individually addressable objectNames, so this stays at "does the diagram render and do the page's
// own commands respond", the same level Lisa/ProLife use for their own scheme/diagram views.

const { test, gui } = require('../fixtures/test');
const { TopologyPage } = require('../pages');
const { refuse } = require('../fixtures/refuse');

test.describe('Topology', () => {
  test.beforeEach(async ({ page }) => {
    await new TopologyPage(page).reload();
  });

  test('renders the diagram', async ({ page }) => {
    const topology = new TopologyPage(page);
    if (!(await topology.isAvailable())) refuse('the Topology page is not in the menu');
    await topology.open();
    await topology.expectOpen();
    // The diagram's own layout (node positions) reflects live agent/service state and is not
    // deterministic across runs/instances - mask the whole scheme, keep the chrome around it.
    const box = await gui.dom.byPath(page, ['TopologySchemeView']).boundingBox();
    const masks = box ? [{ ...box, padding: 0 }] : [];
    await gui.checkScreenshot(page, 'topology-landing', masks);
  });

  test('AutoFit toggles without error', async ({ page }) => {
    const topology = new TopologyPage(page);
    if (!(await topology.isAvailable())) refuse('the Topology page is not in the menu');
    await topology.open();
    await topology.toggleAutoFit();
    await topology.toggleAutoFit();
  });
});
