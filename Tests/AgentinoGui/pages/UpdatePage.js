// UpdatePage - the "Update" page (UpdatePage.qml). A read-only table of the service types installed on
// the agents (ServiceUpdatesList, CServiceUpdateControllerComp): Name, Number of agents, Latest version.
// No commands of its own.

const gui = require('imtcore-gui-testkit/lib/gui');
const { BasePage } = require('imtcore-gui-testkit/pages/BasePage');

class UpdatePage extends BasePage {
  constructor(page) {
    super(page, 'Update');
  }

  expectOpen() {
    return gui.expectVisible(this.page, ['UpdateServicesTable'], 'the Update services table should render');
  }
}

module.exports = { UpdatePage };
