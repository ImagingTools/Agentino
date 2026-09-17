// Service EDITOR (ServiceEditor.qml), reached through an Approved agent's own Services page.
//
// READ-ONLY by construction. Every test here works on a NEW, never-saved service document: the editor
// is opened with the "New" command, inspected, and discarded in afterAll. Nothing is persisted, so
// this file cannot leave a service behind on the instance it ran against.
//
// Why a new document rather than an existing service: a service only exists once someone creates one,
// and creating one needs a PATH - which cannot be typed. ServicePathInput is a
// ServerPathPickerElementView: in the DOM it carries a TextInput and a "Browse..." button but no
// TextField and no DOM <input> at all (ServiceNameInput, by contrast, has TextField -> TextInput), so
// gui.fill() reports 'did not take effect ... got ""'. The path has to come from the Browse dialog,
// which browses the REMOTE agent's filesystem. That belongs in a create/link/start-stop suite, not
// here - see 'the path is a picker, not a typed field' below, which pins the constraint so it stops
// being a surprise.
//
// The agent these run against is seeded by Run-CiTests.ps1, which approves the enrolled test agent
// over GraphQL before Playwright starts. A missing Approved agent is therefore a broken fixture, not
// data variance - hence refuse(), not test.skip().

const { test, expect, gui, newUserPage } = require('../fixtures/test');
const { AgentCollectionPage, AgentEditorPage, ServiceEditorPage } = require('../pages');
const { refuse } = require('../fixtures/refuse');

test.describe('Services / editor (read-only)', () => {
  // One context, one Qt/WASM boot for the whole file: these tests only read, so they share the
  // document the first one opened instead of paying ~11s to bring the app up each.
  let page;
  let service;

  test.beforeAll(async ({ browser }, testInfo) => {
    ({ page } = await newUserPage(browser, testInfo));

    const agents = new AgentCollectionPage(page);
    await agents.reload();
    if (!(await agents.isAvailable())) refuse('the Agents page is not in the menu');
    await agents.open();
    await agents.filterByStatus('approved');
    if (!(await agents.table.hasRows(10000))) {
      refuse('no Approved agent is enrolled, though Run-CiTests.ps1 approves the test agent before the run');
    }
    await agents.selectRow(0);
    await agents.editItem();

    const agentEditor = new AgentEditorPage(page);
    if (!(await agentEditor.hasPage('Services'))) refuse('the agent editor offers no Services page');
    await agentEditor.openServices();
    if (!(await agentEditor.commands.isAvailable('New'))) refuse('the Services collection offers no New command');
    await agentEditor.newService();

    service = new ServiceEditorPage(page);
    await gui.expectVisible(page, ['ServiceNameInput'], 'the New command should open a service editor');
  });

  test.afterAll(async () => {
    if (!page) return;
    // The document is dirty (it is brand new), so closing asks first; closeAllDocumentTabs answers No,
    // which discards it. Nothing this file did reaches the server.
    await gui.closeAllDocumentTabs(page).catch(() => {});
    await page.context().close();
  });

  test('offers the pages a not-yet-saved service has', async () => {
    for (const pageId of ['Information', 'Options', 'Server', 'OutputConnections']) {
      await gui.expectVisible(page, [`Page_${pageId}`], `the ${pageId} page should be offered`);
    }
  });

  // The complement of the test above, and the one that can actually fail: Administration is attached
  // only once serviceTypeId is known, which needs a persisted service (ServiceEditor.qml's
  // isNewService gating). Asserting its ABSENCE is what proves the gating works at all.
  //
  // Only Administration, not Log: the service editor is nested INSIDE the agent editor, which has a
  // Page_Log of its own that is always on screen, so a bare ['Page_Log'] count can never reach 0 and
  // the assertion would be testing the wrong page. Measured: Page_Log is 1 both before and after the
  // service editor opens, Page_Administration is 0 in both.
  test('does not offer Administration until the service is persisted', async () => {
    expect(
      await gui.dom.countVisible(page, ['Page_Administration']),
      'Administration must not be offered for a service that was never saved'
    ).toBe(0);
  });

  test('the Information page shows name, description, path and arguments', async () => {
    await service.openInformation();
    for (const field of ['ServiceNameInput', 'ServiceDescriptionInput', 'ServicePathInput', 'ServiceArgumentsInput']) {
      await gui.expectVisible(page, [field], `${field} should be on the Information page`);
    }
  });

  // Pins the constraint that shapes the whole create-a-service story - see this file's header.
  test('the path is a picker, not a typed field', async () => {
    await service.openInformation();
    await gui.expectVisible(page, ['ServicePathInput'], 'the path control should be on screen');
    expect(
      await gui.dom.countVisible(page, ['ServicePathInput', 'Browse...Button']),
      'the path control offers a Browse button'
    ).toBeGreaterThan(0);

    const typeable = await page.evaluate(() => {
      const root = document.querySelector('[objectName="ServicePathInput"][visible]');
      return root ? root.querySelectorAll('input, textarea').length : -1;
    });
    expect(typeable, 'the path control has no editable DOM input - it is filled by browsing').toBe(0);
  });

  // --- one screenshot per subpage -----------------------------------------------------------------
  //
  // These run BEFORE the tests that type or toggle below, so each captures the subpage in the state a
  // brand-new service actually opens in. Keep them there if this file is ever reordered.
  //
  // Every shot masks Tab1, the document tab: its label is the AGENT's name, which is the machine's
  // computer name ("LAPTOP" here). Unmasked it would make every baseline in this file specific to the
  // machine that generated it and fail on any other, CI included.
  const TAB_MASK = { path: ['Tab1'] };

  test('screenshot: Information', async () => {
    await service.openInformation();
    await gui.checkScreenshot(page, 'service-editor-information', TAB_MASK);
  });

  test('screenshot: Options', async () => {
    await service.openOptions();
    await gui.checkScreenshot(page, 'service-editor-options', TAB_MASK);
  });

  // Renders a deliberate empty state - "Plugin data not loaded / Save the service to automatically
  // load plugin connection settings" - which is exactly why no named host/port controls exist here
  // yet. The shot is the only thing that pins that message.
  test('screenshot: Server', async () => {
    await service.openServer();
    await gui.checkScreenshot(page, 'service-editor-server', TAB_MASK);
  });

  test('screenshot: Output connections', async () => {
    await service.openOutputConnections();
    await gui.checkScreenshot(page, 'service-editor-output-connections', TAB_MASK);
  });

  test('the name field accepts text, unlike the path', async () => {
    await service.openInformation();
    await service.name.fill('GuiTest read-only probe');
    expect(
      await service.name.value(),
      'the name field is a real TextField and must take what was typed'
    ).toContain('GuiTest read-only probe');
  });

  test('Verbose message reveals the tracing level, and toggles back', async () => {
    await service.openOptions();
    await gui.expectVisible(page, ['ServiceVerboseMessageSwitch'], 'the Verbose message switch should be on screen');
    expect(
      await gui.dom.countVisible(page, ['ServiceTracingLevelCombo']),
      'the tracing level is hidden while Verbose message is off'
    ).toBe(0);

    await service.toggleVerboseMessage();
    await gui.expectVisible(page, ['ServiceTracingLevelCombo'], 'turning Verbose message on reveals the tracing level');

    await service.toggleVerboseMessage();
    await gui.expectHidden(page, ['ServiceTracingLevelCombo'], 'turning it back off hides the tracing level again');
  });

  test('the start-script switch reveals its path field, and hides it again', async () => {
    await service.openOptions();
    await gui.expectVisible(page, ['ServiceStartScriptSwitch'], 'the start-script switch should be on screen');
    expect(
      await gui.dom.countVisible(page, ['ServiceStartScriptPathInput']),
      'the start-script path is hidden while the switch is off'
    ).toBe(0);

    await service.startScript.toggle();
    await gui.expectVisible(page, ['ServiceStartScriptPathInput'], 'turning the switch on reveals the start-script path');

    await service.startScript.toggle();
    await gui.expectHidden(page, ['ServiceStartScriptPathInput'], 'turning it back off hides it again');
  });

  // NOTE for whoever adds connection coverage later: for a service that was never saved neither
  // connection subpage renders a named control at all - InputConnectionHostInput /
  // InputConnectionHttpPortInput / InputConnectionWsPortInput (which ServiceEditorPage still declares)
  // are simply absent from the DOM, because the page shows "Plugin data not loaded" instead. Those
  // fields only exist once the service is persisted, so they belong with the create/link scenarios,
  // not here. The two 'screenshot:' tests above are what pins the empty state meanwhile.
});
