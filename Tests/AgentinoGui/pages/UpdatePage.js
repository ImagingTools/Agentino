// UpdatePage - the "Update" page (UpdatePage.qml). A read-only table of the service types installed on
// the agents (ServiceUpdatesList, CServiceUpdateControllerComp): Name, Number of agents, Latest version,
// Auto-update (the StateOk icon when true, an empty cell otherwise).
// The "Operating System" panel on the left (AgentOperatingSystemsList) filters the table: "All" first,
// then the operating system families of the agents (Windows, Linux, macOS, Other), then "Unknown" for
// agents that did not report one.
// No commands of its own. A double click on a service opens its versions page (ServiceVersionsView.qml)
// in a new tab: the versions table (Add / Remove / Install / Set service folder) and the agents panel on the right.
// Versions differ by the operating system family selected in the panel: Set service folder is offered for a family
// only, Add once the folder of the service versions of that family is set.

const gui = require('imtcore-gui-testkit/lib/gui');
const { BasePage } = require('imtcore-gui-testkit/pages/BasePage');

class UpdatePage extends BasePage {
  constructor(page) {
    super(page, 'Update');
  }

  async expectOpen() {
    await gui.expectVisible(this.page, ['UpdateOperatingSystemsPanel'], 'the Operating System panel should render');
    await gui.expectVisible(this.page, ['UpdateServicesTable'], 'the Update services table should render');
  }

  /** Show the services of the agents of all operating systems. */
  selectAllOperatingSystems() {
    return gui.clickButton(this.page, ['UpdateOperatingSystemAllButton']);
  }

  /** Select the first operating system of the panel. */
  selectFirstOperatingSystem() {
    return gui.clickButton(this.page, ['UpdateOperatingSystemButton']);
  }

  /** Double-click the first service of the table: opens its versions page in a new tab. */
  async openFirstServiceVersions() {
    await gui.dom.mouseAreaOf(this.page, ['UpdateServicesTable', 'TableRow_0']).dblclick();
    return this;
  }

  /** The versions page of a service (ServiceVersionsView.qml): the versions table and the agents panel. */
  async expectVersionsOpen() {
    await gui.expectVisible(this.page, ['ServiceVersionsCollectionView'], 'the service versions table should render');
    await gui.expectVisible(this.page, ['ServiceVersionsMetaInfo'], 'the agents panel of the versions page should render');
  }
}

module.exports = { UpdatePage };
