// One way out of "the client did not offer this".
//
// Every spec here signs in as `su`, whose permissions are ['*'] (fixtures/users.js). So when a page,
// a command or a populated collection is not there, nobody is being refused anything - the client or
// the server failed to produce it, and that is a defect, not a reason to stand down.
//
// Same rationale as Lisa/ProLife's own fixtures/refuse.js - see there for the incident that made this
// mandatory (a crashed auth dependency reading as "not offered" and the suite calling that green).
//
// Use test.skip() only where the answer genuinely varies with the data - not for anything a fresh
// Agentino instance guarantees (there is no seeded backup here, see README.md).

function refuse(what) {
  throw new Error(
    `${what} - the suite signs in as "su" with permissions ["*"], so this is the client or the ` +
      'server failing to offer it, not a user who may not have it'
  );
}

module.exports = { refuse };
