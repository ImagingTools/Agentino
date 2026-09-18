// AgentEditorPage - the document tab opened from the Agents collection's Edit command
// (AgentEditor.qml). Only reachable for an Approved agent (see AgentCollectionPage's class doc).
//
// MultiPageView pages (nav items are "Page_<id>"):
//   General   - the agent's own editable fields: name, description, verbose message, tracing level.
//   Services  - the agent's own services, as a nested collection+editor pair (embeds
//               ServiceCollectionView -> ServiceEditor, same commands as a standalone Services page:
//               New/Edit/Remove/Start/Stop). Only added once the agent is persisted (isNewAgent).
//   Log       - the agent's own message log. Only added once persisted.
//   Terminal  - a remote shell to the agent. Only added once persisted AND the user holds
//               "RemoteTerminal" (su does).
// Services/Log/Terminal are added to the MultiPageView asynchronously after the document loads (see
// ensureServicesPage()/ensureLogPage()/ensureTerminalPage()), so hasPage() polls briefly instead of
// checking once.

const gui = require('imtcore-gui-testkit/lib/gui');
const { BasePage } = require('imtcore-gui-testkit/pages/BasePage');
const { TextInput, Table } = require('imtcore-gui-testkit/controls');

class AgentEditorPage extends BasePage {
  constructor(page) {
    // Not a menu page of its own - reuses the Agents collection's command bar/tab machinery.
    super(page, 'Agents');

    this.name = new TextInput(page, ['AgentNameInput']);
    this.description = new TextInput(page, ['AgentDescriptionInput']);
    // Embedded services collection (Services page). Only one Table is on screen once that page is
    // open, so the default (unscoped) Table works.
    this.servicesTable = new Table(page);
  }

  // --- MultiPageView navigation -----------------------------------------------------------------

  async openEditorPage(pageId) {
    await gui.clickButton(this.page, [`Page_${pageId}`]);
    return this;
  }

  openGeneral() {
    return this.openEditorPage('General');
  }
  openServices() {
    return this.openEditorPage('Services');
  }
  openLog() {
    return this.openEditorPage('Log');
  }
  openTerminal() {
    return this.openEditorPage('Terminal');
  }

  /**
   * Whether a lazily-added page (Services/Log/Terminal - see class doc) has appeared yet. Polls
   * because they are attached asynchronously after the document loads, unlike a MultiPageView page
   * declared up front.
   */
  async hasPage(pageId, timeout = 5000) {
    const deadline = Date.now() + timeout;
    for (;;) {
      if ((await gui.dom.countVisible(this.page, [`Page_${pageId}`])) > 0) return true;
      if (Date.now() >= deadline) return false;
      await this.page.waitForTimeout(200);
    }
  }

  // --- General page -----------------------------------------------------------------------------

  async fillGeneral({ name, description } = {}) {
    await this.openGeneral();
    if (name !== undefined) await this.fillAndCommit(this.name, name);
    if (description !== undefined) await this.fillAndCommit(this.description, description);
    return this;
  }

  /** Type into a field and leave it - onEditingFinished (not typing) is what commits the model. */
  async fillAndCommit(field, text) {
    await field.fill(text);
    await gui.clickButton(this.page, ['Page_General']);
    return this;
  }

  /**
   * SwitchCustom directly (its objectName replaces the default "SwitchButton" - see the QML comment
   * next to AgentVerboseMessageSwitch), so this is a plain click, not the Switch control (which
   * expects a wrapper with a nested "SwitchButton").
   */
  async toggleVerboseMessage() {
    await this.openGeneral();
    await gui.click(this.page, ['AgentVerboseMessageSwitch'], { what: 'the Verbose message switch' });
    return this;
  }

  /** Only visible/enabled while Verbose message is on. */
  setTracingLevel(level) {
    return gui.select(this.page, ['AgentTracingLevelCombo'], String(level));
  }

  // --- Services page ----------------------------------------------------------------------------

  newService() {
    return this.runCommand('New');
  }
  editService() {
    return this.runCommand('Edit');
  }
  removeService() {
    return this.runCommand('Remove');
  }
  startService() {
    return this.runCommand('Start');
  }
  stopService() {
    return this.runCommand('Stop');
  }

  async selectServiceByName(name, timeout = 6000) {
    const deadline = Date.now() + timeout;
    for (;;) {
      const names = await this.servicesTable.columnValues('name');
      const index = names.indexOf(name);
      if (index >= 0) {
        await this.servicesTable.selectRow(index);
        return index;
      }
      if (Date.now() >= deadline) throw new Error(`Service row not found: ${name}`);
      await this.page.waitForTimeout(200);
    }
  }
}

module.exports = { AgentEditorPage };
