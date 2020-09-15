@0xd30600b3651feef7;

using Rime = import "/rime_proto.capnp";

struct RimeMessage {
  commit @0 :Rime.Commit;
  context @1 :Rime.Context;
  status @2 :Rime.Status;
}
