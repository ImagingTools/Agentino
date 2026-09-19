// Agents EDITOR tab (AgentEditor.qml), opened via Edit from an Approved agent's row.
//
// Only an Approved agent can be edited (AgentCollectionPage's class doc), and Services/Log/Terminal
// only attach once the agent is persisted - true for every real (enrolled) agent, since this suite
// never creates one through the UI.
//
// READ-ONLY: nothing here is saved. Where a field is edited it is to prove the control takes input,
// and the document is closed without Save afterwards.
//
// Run-CiTests.ps1 enrolls the test agent and approves it over GraphQL before Playwright starts, so a
// missing Approved agent is a broken fixture rather than data variance - refuse(), not test.skip().

const { test, expect, gui } = require('../fixtures/test');
const { AgentCollectionPage, AgentEditorPage } = require('../pages');
const { refuse } = require('../fixtures/refuse');

async function openApprovedAgentEditor(page) {
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
  const editor = new AgentEditorPage(page);
  await editor.openGeneral();
  return editor;
}

test.describe('Agents / editor', () => {
  // --- per-subpage coverage -----------------------------------------------------------------------
  //
  // General and Services get a screenshot; Log and Terminal cannot have a stable one and assert their
  // controls instead - see the note above each.
  //
  // Both shots mask Tab1, the document tab, because its label is the agent's name - which IS the
  // machine's computer name. Unmasked, each baseline would only ever match the machine that generated
  // it. AgentNameInput on the General page is masked for the same reason.
  test.describe('subpage screenshots', () => {
    // fixedWidth, not the tab's own width: the tab is only as wide as the name inside it, so a
    // longer machine name makes the MASK wider and the baseline fails on that boundary alone
    // (measured: 146px for "LAPTOP", 279px for "b035a0a.online-server.cloud" on the build agent).
    const TAB_MASK = { path: ['Tab1'], fixedWidth: 420 };

    test('General', async ({ page }) => {
      const editor = await openApprovedAgentEditor(page);
      await editor.openGeneral();
      await gui.checkScreenshot(page, 'agent-editor-general', [TAB_MASK, { path: ['AgentNameInput'] }]);
      await editor.closeDocument();
    });

    test('Services', async ({ page }) => {
      const editor = await openApprovedAgentEditor(page);
      if (!(await editor.hasPage('Services'))) refuse('the agent editor offers no Services page');
      await editor.openServices();
      await gui.checkScreenshot(page, 'agent-editor-services', TAB_MASK);
      await editor.closeDocument();
    });

    // Log gets NO screenshot either, for the same reason as Terminal below: everything on it is live.
    //
    // Masking the rows was tried and does not rescue it. The masks are computed per run from the rows
    // that happen to be VISIBLE, so a run where the agent logged fewer lines masks a smaller area and
    // exposes table space the baseline has painted black - the shot only matches when the row count
    // matches, which is not a property this page has. (The generated baseline was also ~90% black
    // rectangle, pinning almost nothing.) So this asserts the page's chrome directly: the severity
    // filters, the search box and the column headers.
    test('Log', async ({ page }) => {
      const editor = await openApprovedAgentEditor(page);
      if (!(await editor.hasPage('Log'))) refuse('the agent editor offers no Log page');
      await editor.openLog();
      await gui.expectVisible(page, ['Page_Log'], 'the Log page should be selected');
      for (const severity of ['InfoButton', 'WarningButton', 'ErrorButton', 'CriticalButton', 'VerboseButton']) {
        await gui.expectVisible(page, [severity], `the ${severity} severity filter should be offered`);
      }
      for (const column of ['text', 'timestamp', 'source']) {
        await gui.expectVisible(page, ['TableHeaders', column], `the log should show its ${column} column`);
      }
      await editor.closeDocument();
    });

    // Terminal gets NO screenshot, and that is not an oversight.
    //
    // Once the agent is genuinely connected this page hosts a LIVE shell session: its output carries
    // the agent machine's absolute paths and Windows build number, and the footer runs an idle timer
    // ("Session active - idle warning ~1 min before 15 min close") plus a line counter. None of it is
    // maskable either, because the output area and that footer carry no objectName - only the command
    // input, Send and Ctrl+C do. A pixel baseline of that cannot be stable, so this asserts the page's
    // controls instead.
    //
    // Historical note worth keeping: this page used to read "No shell available - this agent reports
    // no shell that can be started", and an earlier version of this test pinned that as a legitimate
    // empty state. It was not - it was a symptom of the agent being unroutable (see
    // Run-CiTests.ps1's note on ClientId). The screenshot is what caught the difference.
    test('Terminal', async ({ page }) => {
      const editor = await openApprovedAgentEditor(page);
      if (!(await editor.hasPage('Terminal'))) refuse('the agent editor offers no Terminal page');
      await editor.openTerminal();
      await gui.expectVisible(page, ['Page_Terminal'], 'the Terminal page should be selected');
      await gui.expectVisible(page, ['SendButton'], 'a live terminal offers a Send button');
      await gui.expectVisible(page, ['Ctrl+CButton'], '... and an interrupt');
      await editor.closeDocument();
    });
  });

  test.describe('General', () => {
    test('shows and edits name and description', async ({ page }) => {
      const editor = await openApprovedAgentEditor(page);

      await gui.expectVisible(page, ['AgentNameInput'], 'the agent name field should be on screen');
      const before = await editor.name.value();

      await editor.fillGeneral({ description: `GUI test note ${Date.now()}` });
      // Deliberately not saved - see fixtures/refuse.js: this suite shares a real Agentino instance.
      await editor.closeDocument();

      expect(before, 'an enrolled agent always has a name').not.toBe('');
    });

    test('toggling Verbose message reveals the tracing level combo', async ({ page }) => {
      const editor = await openApprovedAgentEditor(page);

      await editor.toggleVerboseMessage();
      await gui.expectVisible(
        page,
        ['AgentTracingLevelCombo'],
        'the tracing level combo should appear once Verbose message is on'
      );
      // Toggle back off, then close without saving.
      await editor.toggleVerboseMessage();
      await editor.closeDocument();
    });
  });

  test.describe('Services (nested collection)', () => {
    test('lists the agent\'s own services', async ({ page }) => {
      const editor = await openApprovedAgentEditor(page);

      if (!(await editor.hasPage('Services'))) refuse('the agent editor offers no Services page, though an enrolled agent is persisted');

      await editor.openServices();
      await gui.expectVisible(page, ['Table'], 'the embedded services table should render');
      await editor.closeDocument();
    });
  });

  test.describe('Log', () => {
    test('opens without error', async ({ page }) => {
      const editor = await openApprovedAgentEditor(page);

      if (!(await editor.hasPage('Log'))) refuse('the agent editor offers no Log page, though an enrolled agent is persisted');

      await editor.openLog();
      await gui.expectVisible(page, ['Page_Log'], 'the Log page should be selected');
      await editor.closeDocument();
    });
  });

  test.describe('Terminal', () => {
    // su holds "RemoteTerminal" (permissions ['*']) - see AgentEditorPage's class doc. Only the tab's
    // presence is asserted; no command is actually typed against a real remote shell.
    test('is offered to a user holding RemoteTerminal', async ({ page }) => {
      const editor = await openApprovedAgentEditor(page);

      if (!(await editor.hasPage('Terminal'))) refuse('the agent editor offers no Terminal page, though an enrolled agent is persisted');

      await editor.openTerminal();
      await gui.expectVisible(page, ['Page_Terminal'], 'the Terminal page should be selected');
      await editor.closeDocument();
    });
  });
});
