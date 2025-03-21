# General:
- This is a modified version of Rubén Martínez's project code submitted to nsnam.org located at: https://www.nsnam.org/wiki/GSOC2014LTP (date: 17 Feb 2015)
- Intended for use with Bundle Protocol v7 emulation from: https://github.com/alexk1vt/ns-3-bundle-protocol-v7.git
- Was developed to operate with NS-3 version 3.34 - has not been tested with any other version of NS-3

# How to Use:
- Bundle Protocol supports two L4 protocols: TCP and LTP (LTP rides on top of UDP with a server port number of 1113)
- Nodes must have support L4 protocol installed prior to installing bundle protocol. To use LTP, nodes must have LTP installed and linked with all LTP ‘remote peers’ they will be communicating with.

# Ltp-Protocol Module Description:
## Addressing:
- Nodes are associated with uint64_t Ltp Engine Ids; numbering normally starts at 0 and increases sequentially.

## Routing:
- Ltp nodes reference an Ltp Ip Resolution Table to associate Engine Ids to IP addresses.
- Ltp Ip Resolution Table is created by the user with specific attributes
- An Ltp Engine Id can have only a single bound IP address
- LtpHelper binds node Ltp Engine Id to interface 1 IP address
  - If node has alternate interface IP address that should be associated, pre-emptively perform the binding prior to installing Ltp using LtpHelper

## Remote Peers:
- Ltp nodes can only communicate with other Ltp nodes that are configured as ‘remote peers’.
- If Ltp Node has \>1 remote peers, packet source IP must match one of the remote peers
- When using LtpHelper.InstallAndLink (NodeContainer c), nodes are assigned remote peers in a single direction loop (ie, 3 node scenario: Node0 has remote peer of Node1, Node 1 has remote peer of Node2, Node2 has remote peer of Node0)
  - This has little utility and normally requires additional linking afterwards for bi-directional communication (ie, link Node2 with Node1 and link Node1 with Node0)

## Classes and Methods of Interest:
```cpp
LtpHelper::
  SetAttributes(“ReportSegmentRtxLimit”, UintergerValue (20),
                “RetransCyclelimit”, UintergerValue (20),
                “OneWayLightTime”, StringValue (“20ms”));
  SetLtpIpResolutionTable (LtpIpResolutionTable ltpRouting);
  SetBaseLtpEngineId (uint64_t baseLtpEngineId);
  InstallAndLink (NodeContainer c);
  Link (Ptr<Node> node, uint64_t remotePeer);

LtpIpResolutionTable::
  CreateObjectWithAttributes<ns3::ltp::LtpIpResolutionTable>(“Addressing”, StringValue (“Ipv4”));
  CreateObjectWithAttributes<ns3::ltp::LtpIpResolutionTable>(“Addressing”, StringValue (“Ipv4”));
  AddBinding (uint64_t LtpEngineId, Ipv4Address dstAddr, uint16_t port); // creates a binding to associate the given LtpEngingId with the given IP address and port number. Call this prior to LtpHelper::InstallAndLink(..)

LtpProtocol::
  GetLocalEngineId ();
  SetRedDataMode (uint8_t redMode); 
  // Sets the red data mode of the installed LTP module 
  //   (0 – no red data; 
  //   1 – only primary block is red data; 
  //   2 – all but payload block is red data; 
  //   3 – entire bundle is red data)
```
#### Constraints:
- Any green data is lost when sending to a node that is not available to receive it on initial transmission. Bundle protocol conducts rudimentary connectivity checks prior to transmitting bundles via LTP to try preventing unnecessary loss of green data.
- No transmission cancellation mechanisms implemented.
- Unresolved memory leaks identified by Valgrind.
