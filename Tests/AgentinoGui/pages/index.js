// Agentino's own page objects, plus the generic ImtCore ones re-exported so a spec has one import.
//
// The suite covers exactly two pages of AgentinoServer's own UI: Topology, and Agents - and inside an
// agent, its Services. Two areas are deliberately out of scope and have no page objects here:
// Administration (this server does not implement the document service its editors need) and
// AgentinoAgent's own web UI (a separate app; the agent process still runs, because the suite needs it
// to enroll, but nothing drives its interface).

const { BasePage, CollectionPage } = require('imtcore-gui-testkit/pages');

const { AgentCollectionPage, STATUS_FILTERS, MASK_COLUMNS } = require('./AgentCollectionPage');
const { AgentEditorPage } = require('./AgentEditorPage');
const { ServiceEditorPage } = require('./ServiceEditorPage');
const { TopologyPage } = require('./TopologyPage');

module.exports = {
  BasePage,
  CollectionPage,

  AgentCollectionPage,
  STATUS_FILTERS,
  MASK_COLUMNS,
  AgentEditorPage,
  ServiceEditorPage,
  TopologyPage,
};
