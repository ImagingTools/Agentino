// Agents COLLECTION view (AgentCollectionView.qml).
//
// Not a standard Imt collection - no server-side sort, and status is filtered via six dedicated
// toolbar buttons instead of the generic FilterPanel combo (see AgentCollectionPage's class doc for
// why), so this does not use imtcore-gui-testkit's defineCollectionSpec() and is written directly.
//
// Agents are not something this suite can create: a row only exists once a real AgentinoAgent process
// enrolls with the server. So every scenario that needs a row of a particular status first asks the
// table for one and skips (not refuses - this genuinely varies with which agents happen to be running
// against this instance) when there isn't one.

const { test, expect, gui, newUserPage } = require('../fixtures/test');
const { AgentCollectionPage, STATUS_FILTERS } = require('../pages');
const { refuse } = require('../fixtures/refuse');

test.describe('Agents / collection', () => {
  test.describe('cold load', () => {
    test.beforeEach(async ({ page }) => {
      await new AgentCollectionPage(page).reload();
    });

    test('landing', async ({ page, gui: g }) => {
      const agents = new AgentCollectionPage(page);
      if (!(await agents.isAvailable())) refuse('the Agents page is not in the menu');
      await agents.open();
      await agents.expectOpen();
      await g.checkScreenshot(page, 'agents-landing', await agents.masks());
    });

    test('status filter buttons are all present', async ({ page }) => {
      const agents = new AgentCollectionPage(page);
      await agents.open();
      for (const objectName of Object.values(STATUS_FILTERS)) {
        await gui.expectVisible(page, [objectName], `status filter "${objectName}" should be offered`);
      }
    });
  });

  test.describe.serial('status filters', () => {
    let page, agents;

    test.beforeAll(async ({ browser }, testInfo) => {
      ({ page } = await newUserPage(browser, testInfo));
      agents = new AgentCollectionPage(page);
      // newUserPage() only opens a BLANK page - without this navigation the app is never loaded, and
      // isAvailable() then waits out its full 30s for a MenuPanel that cannot arrive, failing the
      // whole serial block with "the left menu never appeared".
      await agents.reload();
      if (!(await agents.isAvailable())) refuse('the Agents page is not in the menu');
      await agents.open();
    });

    test.afterAll(async () => {
      if (page) await page.context().close();
    });

    for (const bucket of ['all', 'pending', 'approved', 'suspended', 'rejected', 'revoked']) {
      test(`"${bucket}" filters the table without error`, async () => {
        await agents.filterByStatus(bucket);
        await agents.expectOpen();
      });
    }

    // There is deliberately NO free-text search test here. The Agents collection does not have one:
    // measured on a loaded page, FilterPanel and SearchTextInput are absent from the DOM entirely
    // (count 0, not merely hidden), and toggling FilterVisible does not bring them in - the page
    // offers only the six status buttons above. CollectionPage.search() targets
    // [FilterPanel > SearchTextInput], so a search test here can only ever fail with
    // "FilterPanel: MISSING", which is what it did. Restore this if the page ever grows a search box.
  });

  // --- enrollment workflow --------------------------------------------------------------------------
  //
  // Approve/Reject/Suspend/Resume/Revoke/Reset each only enable for a specific current status (see
  // AgentCollectionViewCommandsDelegate.qml, mirrored in AgentCollectionPage's class doc). Each test
  // below finds a row already in the right status and skips if none exists, rather than fabricating
  // one - only a real Agent process changes an agent's status by connecting/enrolling.
  test.describe('enrollment decisions', () => {
    test.beforeEach(async ({ page }) => {
      await new AgentCollectionPage(page).reload();
    });

    async function firstRowWithStatus(agents, bucket) {
      await agents.open();
      await agents.filterByStatus(bucket);
      if (!(await agents.table.hasRows(4000))) return false;
      await agents.selectRow(0);
      return true;
    }

    test('a Pending agent offers Approve and Reject, not Edit', async ({ page }) => {
      const agents = new AgentCollectionPage(page);
      if (!(await firstRowWithStatus(agents, 'pending'))) {
        test.skip(true, 'no Pending agent is currently enrolled against this instance');
        return;
      }
      expect(await agents.commands.isAvailable('Approve')).toBe(true);
      expect(await agents.commands.isAvailable('Reject')).toBe(true);
      expect(await agents.commands.isAvailable('Edit')).toBe(false);
      await gui.checkScreenshot(page, 'agents-pending-selected', await agents.masks());
    });

    // Only the POSITIVE half is assertable here, and that is a property of the app, not a shortcut.
    //
    // AgentCollectionViewCommandsDelegate.qml gates these with setCommandIsEnabled(...) - it disables
    // commands, it never hides or removes them. Enabled-state is painted on the canvas and reaches the
    // DOM in no form at all: a command button carries only class/objectname/visible/style (measured on
    // an Approved agent). So "Resume is not offered" is unobservable from here - isAvailable() answers
    // visibility, and a disabled Resume is still visible. The old assertion
    // `isAvailable('Resume')).toBe(false)` therefore tested nothing it claimed to and failed as soon
    // as a real Approved agent existed to select.
    //
    // Measured on the same agent, visibility does not track enablement either: Revoke is enabled by
    // the delegate for an Approved agent yet is NOT visible on the bar (it moves into the "..."
    // overflow), which is why the Revoke check below asks for DOM existence rather than visibility.
    test('an Approved agent offers Edit, Suspend and Revoke', async ({ page }) => {
      const agents = new AgentCollectionPage(page);
      if (!(await firstRowWithStatus(agents, 'approved'))) {
        refuse('no Approved agent is enrolled, though Run-CiTests.ps1 approves the test agent before the run');
      }
      expect(await agents.commands.isAvailable('Edit'), 'Edit is offered for an Approved agent').toBe(true);
      expect(await agents.commands.isAvailable('Suspend'), 'Suspend is offered for an Approved agent').toBe(true);
      expect(
        await gui.dom.countAny(page, ['CommandsView', 'RevokeButton']),
        'Revoke exists on the bar for an Approved agent, visible or in the overflow'
      ).toBeGreaterThan(0);
    });

    // The enrolled test agent must read "Connected", not merely be Approved - and this is worth its own
    // assertion rather than being left to the landing screenshot, because the failure it guards is a
    // quiet one. An agent whose id does not route looks entirely healthy from its own side (its log
    // says "WebSocket connected") while the server cannot reach it: the column reads "Disconnected",
    // the agent's service list silently stays empty because ServicesList never arrives, and the
    // Terminal page claims no shell is available. See Run-CiTests.ps1's note on ClientId for the two
    // code paths that disagree when it is empty.
    test('the enrolled agent reads Connected, not just Approved', async ({ page }) => {
      const agents = new AgentCollectionPage(page);
      await agents.reload();
      if (!(await agents.isAvailable())) refuse('the Agents page is not in the menu');
      await agents.open();
      await agents.filterByStatus('approved');
      if (!(await agents.table.hasRows(10000))) {
        refuse('no Approved agent is enrolled, though Run-CiTests.ps1 approves the test agent before the run');
      }

      const statuses = await agents.table.columnValues('status');
      expect(
        statuses[0],
        'the agent the harness started and approved must be reachable from the server, not merely enrolled'
      ).toBe('Connected');
    });

    test('a Suspended agent offers Resume and Revoke, not Edit', async ({ page }) => {
      const agents = new AgentCollectionPage(page);
      if (!(await firstRowWithStatus(agents, 'suspended'))) {
        test.skip(true, 'no Suspended agent is currently enrolled against this instance');
        return;
      }
      expect(await agents.commands.isAvailable('Resume')).toBe(true);
      expect(await agents.commands.isAvailable('Revoke')).toBe(true);
      expect(await agents.commands.isAvailable('Edit')).toBe(false);
    });

    test('a Rejected or Revoked agent offers Reset only', async ({ page }) => {
      const agents = new AgentCollectionPage(page);
      let found = await firstRowWithStatus(agents, 'rejected');
      if (!found) found = await firstRowWithStatus(agents, 'revoked');
      if (!found) {
        test.skip(true, 'no Rejected/Revoked agent is currently enrolled against this instance');
        return;
      }
      expect(await agents.commands.isAvailable('Reset')).toBe(true);
      expect(await agents.commands.isAvailable('Edit')).toBe(false);
      expect(await agents.commands.isAvailable('Approve')).toBe(false);
    });

    // Actually flips a real agent's enrollment state - Reset (Rejected/Revoked -> Pending) is the one
    // decision that is safe to also reverse in the same test (Approve/Reject/Suspend/Resume/Revoke are
    // not: there is no guaranteed way back to the exact prior status without knowing it in advance).
    test('Reset returns a Rejected/Revoked agent to Pending', { tag: '@mutating' }, async ({ page }) => {
      const agents = new AgentCollectionPage(page);
      let found = await firstRowWithStatus(agents, 'rejected');
      if (!found) found = await firstRowWithStatus(agents, 'revoked');
      if (!found) {
        test.skip(true, 'no Rejected/Revoked agent is currently enrolled against this instance');
        return;
      }
      await agents.reset();
      await agents.filterByStatus('pending');
      expect(await agents.table.hasRows(6000), 'the reset agent should now show under Pending').toBe(true);
    });
  });

  test.describe('Remove', () => {
    test.beforeEach(async ({ page }) => {
      await new AgentCollectionPage(page).reload();
    });

    test('asks for confirmation', async ({ page }) => {
      const agents = new AgentCollectionPage(page);
      if (!(await agents.isAvailable())) refuse('the Agents page is not in the menu');
      await agents.open();
      if (!(await agents.table.hasRows())) refuse('the Agents collection came back empty');
      await agents.selectRow(0);
      if (!(await agents.commands.isAvailable('Remove'))) {
        test.skip(true, 'Remove is not offered for the current selection');
        return;
      }
      await agents.removeItem();
      await gui.expectVisible(page, ['Dialog'], 'removing an agent should ask first');
      await gui.clickButton(page, ['NoButton']);
      await gui.expectHidden(page, ['Dialog'], 'the confirm should close');
    });
  });
});
