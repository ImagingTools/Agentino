// ServiceEditorPage - the document tab opened from a Services collection's New/Edit (either the
// standalone Services page inside AgentEditor's "Services" subpage, or Topology's own list) -
// ServiceEditor.qml.
//
// MultiPageView pages (nav items are "Page_<id>"; "General" and "Connections" are collapsible
// categories with no page component of their own - only their subpages are real):
//   General
//     Information   - name, description, path, arguments
//     Options       - autostart / verbose message (+ tracing level) / start & stop scripts
//   Connections
//     Server              - the service's own listen socket (host / http port / ws port / secure)
//     OutputConnections   - the other services this one talks to (a checkable table, not covered here)
//   Administration  - embedded generic AdministrationView (support tickets etc.), added only once
//                     serviceTypeId is known (i.e. not for a brand-new, not-yet-saved service).
//   Log             - the service's own message log, same isNewService gating as Administration.

const gui = require('imtcore-gui-testkit/lib/gui');
const { BasePage } = require('imtcore-gui-testkit/pages/BasePage');
const { TextInput, Switch, ComboBox, Table } = require('imtcore-gui-testkit/controls');

class ServiceEditorPage extends BasePage {
  constructor(page) {
    // Not a menu page of its own - reuses whichever collection's command bar/tab machinery opened it.
    super(page, 'Agents');

    // Information
    this.name = new TextInput(page, ['ServiceNameInput']);
    this.description = new TextInput(page, ['ServiceDescriptionInput']);
    this.path = new TextInput(page, ['ServicePathInput']);
    this.arguments = new TextInput(page, ['ServiceArgumentsInput']);

    // Options
    this.autostart = new Switch(page, ['ServiceAutostartSwitch']);
    this.verboseMessage = new Switch(page, ['ServiceVerboseMessageSwitch']);
    this.tracingLevel = new ComboBox(page, ['ServiceTracingLevelCombo']);
    this.startScript = new Switch(page, ['ServiceStartScriptSwitch']);
    this.startScriptPath = new TextInput(page, ['ServiceStartScriptPathInput']);
    this.stopScript = new Switch(page, ['ServiceStopScriptSwitch']);
    this.stopScriptPath = new TextInput(page, ['ServiceStopScriptPathInput']);

    // Connections / Server
    this.host = new TextInput(page, ['InputConnectionHostInput']);
    this.httpPort = new TextInput(page, ['InputConnectionHttpPortInput']);
    this.wsPort = new TextInput(page, ['InputConnectionWsPortInput']);
    this.outputConnections = new Table(page, ['OutputConnectionsTable']);
  }

  // --- MultiPageView navigation -----------------------------------------------------------------

  async openEditorPage(pageId) {
    await gui.clickButton(this.page, [`Page_${pageId}`]);
    return this;
  }

  openInformation() {
    return this.openEditorPage('Information');
  }
  openOptions() {
    return this.openEditorPage('Options');
  }
  openServer() {
    return this.openEditorPage('Server');
  }
  openOutputConnections() {
    return this.openEditorPage('OutputConnections');
  }
  openAdministration() {
    return this.openEditorPage('Administration');
  }
  openLog() {
    return this.openEditorPage('Log');
  }

  /** Whether a lazily-added page (Administration/Log - only once serviceTypeId is known) has appeared. */
  async hasPage(pageId, timeout = 5000) {
    const deadline = Date.now() + timeout;
    for (;;) {
      if ((await gui.dom.countVisible(this.page, [`Page_${pageId}`])) > 0) return true;
      if (Date.now() >= deadline) return false;
      await this.page.waitForTimeout(200);
    }
  }

  // --- Information page -------------------------------------------------------------------------

  async fillInformation({ name, description, path, args } = {}) {
    await this.openInformation();
    if (name !== undefined) await this.fillAndCommit(this.name, name);
    if (description !== undefined) await this.fillAndCommit(this.description, description);
    if (path !== undefined) await this.fillAndCommit(this.path, path);
    if (args !== undefined) await this.fillAndCommit(this.arguments, args);
    return this;
  }

  async selectOutputConnection(index = 0) {
    await this.openOutputConnections();
    await this.outputConnections.toggleRowCheck(index);
    return this;
  }

  /** Type into a field and leave it - onEditingFinished (not typing) is what commits the model. */
  async fillAndCommit(field, text) {
    await field.fill(text);
    await gui.clickButton(this.page, ['Page_Information']);
    return this;
  }

  // --- Options page ------------------------------------------------------------------------------

  async toggleAutostart() {
    await this.openOptions();
    await this.autostart.toggle();
    return this;
  }

  /** Tracing level combo only shows/enables once Verbose message is on. */
  async toggleVerboseMessage() {
    await this.openOptions();
    await this.verboseMessage.toggle();
    return this;
  }

  setTracingLevel(level) {
    return this.tracingLevel.select(String(level));
  }

  // --- Server (Connections) page ------------------------------------------------------------------

  async fillServerConnection({ host, httpPort, wsPort } = {}) {
    await this.openServer();
    if (host !== undefined) await this.host.fill(host);
    if (httpPort !== undefined) await this.httpPort.fill(String(httpPort));
    if (wsPort !== undefined) await this.wsPort.fill(String(wsPort));
    return this;
  }

  /**
   * The service's own listen socket "secure" toggle. Assigned directly on the underlying switch
   * control (its objectName replaces the default "SwitchButton", same pattern as AgentEditorPage's
   * verbose-message switch), so this is a plain click, not the Switch control.
   */
  async toggleSecureConnection() {
    await this.openServer();
    await gui.click(this.page, ['InputConnectionSecureSwitch'], { what: 'the connection Secure switch' });
    return this;
  }

  // --- editor commands --------------------------------------------------------------------------

  undo() {
    return this.runCommand('Undo');
  }
  redo() {
    return this.runCommand('Redo');
  }
  startService() {
    return this.runCommand('Start');
  }
  stopService() {
    return this.runCommand('Stop');
  }
}

module.exports = { ServiceEditorPage };
