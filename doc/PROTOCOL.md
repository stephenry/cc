# "CC" Protocol Specification

CC implements a standard MOESI protocol using the ARM ACE 4.0
Specification.

## Protocol

### Messages

#### Coherency Start/End Messages (CohSrt/CohEnd)

[Message Definition](../src/protocol.h)

The Coherency Start (CohSrt) message is issued by the cache controller
instance to the home directory to indicate the start of a coherent
transaction. The message performs the following:

* Indicates the start of a coherent transaction and reserves a slot
  within both the directory and cache controllers transaction tables.

As the CohSrt does not contain information relating to the coherent
command, it is typically succeeded by a corresponding CohCmd message.

The Coherence End (CohEnd) message is issued by the directory to the
home cache controller to indicate completion of the coherent
transaction. The message performs the following:

* Indicates the overall result of the coherency operation as computed
  by the directory.
* Returns a credit back to the initiator cache controller to indicate
  that the a new CohSrt can be issued.
* Frees any resources allocated within the directory which may have
  been reserved by the transaction.

#### Coherency Command/Response Messages (CohCmd)

[Message Definition](../src/protocol.h)

The coherency command message communicates the desired coherence
command between the L2 cache and the directory which owns the
line. Once consumed, the CohCmdRsp message is issued to the initiating
cache controller to return the credit associated CohCmd.

#### Coherency Snoop/Response Messages (CohSnp)

[Message Definition](../src/protocol.h)

Coherency Snoop and Response messages model the notion of a directory
initated snoop command and the associated response indicating the
result of the requested snoop operation. The messages themselves
closely model the behavior of snoop commands and responses as defined
in the ACE specification.

#### Data Transfer Messages (Dt)

[Message Definition](../src/mem.h)

The Data Transfer (Dt) message models the notion of the transfer of a
single cache line worth of data between two communicating agents. The
message does not itself sufficient state to either indicate the
address of the line nor the final state and instead relies upon this
state to have been communicated to the receiving agent by another
message class. The Data Transfer response (DtRsp) message is used to
return a credit back to the sender.

LLC messages do not represent coherence traffic.


#### LLC Fill/Evict/Put Messages (LLC)

[Message Definition](../src/llc.h)

Last Level Cache (LLC) messages are used to issue commands to instruct
the LLC to:

* Fill a cache line by fetching one Dt's worth of data from the memory
  controller which owns the addressed line.
* Evict a cache line by writing a cache line to the memory controller.
* Put a cache line by transferring a previously installed LLC cache
  line to the agent indicated in the message body.

LLC messages do not represent coherence traffic.

#### L2 Command/Response Messages (L2Cmd)

[Message Definition](../src/l2cache.h)

L2 Commands are issued by L1 to fill a cache line in a particular
requested state or notify upon an eviction.

#### L1 Command/Reponse Messages (L1Cmd)

[Message Definition](../src/l1cache.h)

L1 commands roughly approximate the notion of a Load/Store instruction
issued by a CPU to its associated L1 cache instance. The cache respond
directly to an initial Load or a Store message with a response message
which contains the line of interest.


### Protocol Examples

#### Load to a Unique Cache Line

1. CPU initiates a Load to a cache line.
2. L1 consumes L1Cmd issued by child CPU and identifies that the line
   is not present in its cache. L1 issues a L2Cmd to its owning L2
   instance which instructs L2 that it requires the line in a Shared
   state. As the initiating L1 command cannot complete, the message
   queue through which it originated is marked as blocked until
   completion of the current transaction.
3. L2 consumes L2Cmd issued by child L1 and identifies that the line
   is not present in its cache. L2 initiates a fill operation by
   issuing a ReadShared AceCmd to its AR channel.
4. The AceCmd is consumed by the owning cache controller instance and
   a new coherent transaction is started. A CohSrt message is issued
   to the directory which is home to the addressed cache
   line. Immediately after the CohSrt, a CohCmd is issued to the same
   directory containing the ReadShared opcode.
5. The directory consumes the CohSrt and CohCmd messages and
   identifies that the requested line is not present in any cache
   within the system. A line is allocated within the directory and a
   LLC fill command is issued to the LLC to instruct for it to read
   the line from the memory controller which owns the line.
6. Notification is received by the LLC indicating that the line is now
   present in the LLC. The directory instructs the LLC to forward the
   cache line to the requesting agent (CC). The line is transfered to
   the agent and a response is received by the directory indicating
   that the operation has completed.
7. The originator CC is now in receipt of one Dt which contains the
   cache line. The directory computes the final coherence result which
   indicates that the requester now has the line in an Exclusive state
   (even though it had only originally been requested in the Shared
   state) and sends the terminal CohEnd message to the originator CC.
8. The CC receives the CohEnd message and forwards a AceCmdRsp
   response to its owning L2. The message indicates that the line is
   passed as not shared and not dirty.
9. The line is installed in the L2 in the Exclusive state and the
   originator L1 is indicated as the owner of the line. A L2CmdRsp
   message is issued to the L1 containing the line state.
10. The line is installed in the L1 in the Exclusive state. The
    message queue containing the initiating Load instruction is marked
    unblocked.
11. The load instruction is rearbitrated and issued to L1. As the line
    is now present in the L1, the load completes and the result passed
    to the L1.
12. The originator CPU receives the L1CmdRsp from its L1 and the
    instruction commits. The overall transaction is complete.

#### Load to a Shared Cache Line

#### Store to a Shared Cache Line

## Agents

The term agent roughly encapsulates the notion of some communicating
entity which participates within the overall simulation context.

The following agents are defined:

### Directory Agent

#### Last-Level Cache (LLC)

The Last Level Cache (LLC) contains the cache line state owned by the
directory. As line allocation is managed directory by the owning
directory, the purpose of the LLC is simply to model the cost
associated with fetching a line from memory, and not through
intervention. Although multiple instances of LLC may exist with in a
simulation, there always remains a one-to-one correspondence between
the LLC and its controlling directory.

#### Memory Controller (MC)

The memory controller agent models the concept of some costly external
interface to external memory. It is accessed by a LLC in response to
fill and writeback commands. No other agents within the simulation
access the memory controller agent and all line accesses are mediated
by the LLC.

### CPU Cluster (CC)

#### Cache Controller Agent

The Cache Controller (CC) Agent sits between the L2 cache instance and
the system interconnect. Its major role is to translate ACE messages
as they originate from the L2 and to convert coherence messages as
they are issued to or received from the system interconnect. The cache
controller is used to retain coherence traffic received from snooped
agents (typically Dt messages which arrive because of intervention)
and to form the final coherence result message back to the L2
(equivalent to the R or B channels) to indicate the final installed
state of the line.

#### L2 Cache

The L2 Cache (L2) Agent sits between the Cache Controller block and
some number of child L1 instance.

#### L1 Cache

The L1 Cache (L2) Agents sits between the L2 cache instance and an
associated L1 cache instance (L1). The L1 communicates with its owning
L2 instance to request lines to be filled, and responds to its child
CPU instance.

The L1 implements a write-through policy such that all state are seen
by the corresponding L2. Similarly, line demotions or invalidations
which occur in L2 in response to inbound snoop commands are reflected
instantaneously in the associated L1 instance. This is quite an
important simplification in terms of the simulators implementation as
a true hardware implementation would require for operations to L1 and
L2 to remain in a speculative state until their result had been
finalized. Although the impact of misspeculation in the load/store
units causes some penalty associated with the pipeline flush and
consequent replay of the CPU instruction sequence, this can be
neglected within the context of a simulator as misspeculations are
assumed to be a relatively infrequent event.

#### CPU

The CPU Agent models the notion of a simple CPU data path which
periodically emits load and store instructions to certain regions of
memory. The CPU agent communicates to the system by issuing L1Cmd
messages to its associated L1 instance and has no other means by which
to communicate with other agents in the system.