// TopologyPage - the "Topology" page (TopologyPage.qml). A live diagram of every agent's services
// (topologyViewModel.loadTopology(), refreshed by "OnServiceStatusChanged"/topology subscriptions -
// see /memories/repo/agentino-build-notes.md for the status-plumbing history). Selecting a node drives
// the same Start/Stop/Edit/Remove commands as the Services collection; Remove is refused while the
// owning agent is disconnected (canRemoveSelectedService()/showCannotRemoveWhileDisconnected()).

const gui = require('imtcore-gui-testkit/lib/gui');
const { BasePage } = require('imtcore-gui-testkit/pages/BasePage');

class TopologyPage extends BasePage {
  constructor(page) {
    super(page, 'Topology');
  }

  expectOpen() {
    return gui.expectVisible(this.page, ['TopologySchemeView'], 'the Topology diagram should render');
  }

  async selectServiceNode() {
    const box = await gui.dom.byPath(this.page, ['TopologySchemeView']).boundingBox();
    if (!box) throw new Error('Topology diagram has no selectable bounds');
    await this.page.mouse.click(box.x + box.width / 2, box.y + box.height / 2);
    return this;
  }

  toggleAutoFit() {
    return this.runCommand('AutoFit');
  }

  editSelectedService() {
    return this.runCommand('Edit');
  }
  removeSelectedService() {
    return this.runCommand('Remove');
  }
  startSelectedService() {
    return this.runCommand('Start');
  }
  stopSelectedService() {
    return this.runCommand('Stop');
  }
}

module.exports = { TopologyPage };
