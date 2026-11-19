// UpdatePage - the "Update" page (UpdatePage.qml). A read-only table of the service types installed on
// the agents (ServiceUpdatesList, CServiceUpdateControllerComp): Name, Number of agents, Latest version,
// Auto-update (the StateOk icon when true, an empty cell otherwise).
// The "Operating System" panel on the left (AgentOperatingSystemsList) filters the table: "All" first,
// then the operating system families of the agents (Windows, Linux, macOS, Other), then "Unknown" for
// agents that did not report one.
// No commands of its own.

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
}

module.exports = { UpdatePage };
