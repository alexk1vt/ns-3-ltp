/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Universitat Autnoma de Barcelona
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Rubn Martnez <rmartinez@deic.uab.cat>
 */

/*
Updates made by: Alexander Kedrowitsch <alexk1@vt.edu>

Aggregate changes for commits in range: 20247a4..275be8c

Modified/Added Function: InstallAndLink
  - Related commit message: Made a number of changes; big one is ensuring Ltp receives on correct CLA associated with remote peer.
  - Related commit message: Functional for multi-hop scenarios (uni-directional). Red code still not working for bundle-protocols. Added link(..) method for ltp-helper for bi-directional scenarios but havent yet tested.

Modified/Added Function: Install
  - Related commit message: Functional for multi-hop scenarios (uni-directional). Red code still not working for bundle-protocols. Added link(..) method for ltp-helper for bi-directional scenarios but havent yet tested.

Modified/Added Function: Link
  - Related commit message: Made a number of changes; big one is ensuring Ltp receives on correct CLA associated with remote peer.

*/

#include "ltp-protocol-helper.h"
#include "ns3/object.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/enum.h"
#include <ns3/log.h>

NS_LOG_COMPONENT_DEFINE ("LtpHelper");


namespace ns3 {
namespace ltp {

LtpProtocolHelper::LtpProtocolHelper ()
  : m_startTime (Seconds (0)),
    m_ltpid (0),
    m_resolutionTable (0)
{
  NS_LOG_FUNCTION (this);
  m_claFactory.SetTypeId (LtpUdpConvergenceLayerAdapter::GetTypeId ());   // Default UDP
  m_ltpFactory.SetTypeId (LtpProtocol::GetTypeId ());
}

void
LtpProtocolHelper::SetAttributes (std::string n1, const AttributeValue &v1,
                                  std::string n2, const AttributeValue &v2,
                                  std::string n3, const AttributeValue &v3,
                                  std::string n4, const AttributeValue &v4,
                                  std::string n5, const AttributeValue &v5,
                                  std::string n6, const AttributeValue &v6,
                                  std::string n7, const AttributeValue &v7,
                                  std::string n8, const AttributeValue &v8,
                                  std::string n9, const AttributeValue &v9)
{

  m_ltpFactory.Set (n1, v1);
  m_ltpFactory.Set (n2, v2);
  m_ltpFactory.Set (n3, v3);
  m_ltpFactory.Set (n4, v4);
  m_ltpFactory.Set (n5, v5);
  m_ltpFactory.Set (n6, v6);
  m_ltpFactory.Set (n7, v7);
  m_ltpFactory.Set (n8, v8);
  m_ltpFactory.Set (n9, v9);
}


void
LtpProtocolHelper::SetConvergenceLayerAdapter (std::string type,
                                               std::string n1, const AttributeValue &v1,
                                               std::string n2, const AttributeValue &v2,
                                               std::string n3, const AttributeValue &v3,
                                               std::string n4, const AttributeValue &v4,
                                               std::string n5, const AttributeValue &v5,
                                               std::string n6, const AttributeValue &v6,
                                               std::string n7, const AttributeValue &v7,
                                               std::string n8, const AttributeValue &v8,
                                               std::string n9, const AttributeValue &v9)
{
  m_claFactory.SetTypeId (type);
  m_claFactory.Set (n1, v1);
  m_claFactory.Set (n2, v2);
  m_claFactory.Set (n3, v3);
  m_claFactory.Set (n4, v4);
  m_claFactory.Set (n5, v5);
  m_claFactory.Set (n6, v6);
  m_claFactory.Set (n7, v7);
  m_claFactory.Set (n8, v8);
  m_claFactory.Set (n9, v9);
}


void
LtpProtocolHelper::InstallAndLink (NodeContainer c) 
{
  NS_LOG_FUNCTION (this);

  uint64_t base_addr = m_ltpid;
  uint32_t numNode = 0;
  uint32_t nodeCnt = c.GetN ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      if (++numNode < nodeCnt)
      //if (numNode++ % 2 == 0) // This assumes numNode stays lockstep with node EngineIds (AlexK.)
        {
          m_claFactory.Set ("RemotePeer", UintegerValue (m_ltpid + 1)); // setting second node EngineId as remote peer (AlexK.)
          NS_LOG_FUNCTION(this << " Node number " << numNode << " out of " << nodeCnt << " is being assigned RemotePeer " << m_ltpid + 1);
          //m_claFactory.Set ("RemotePeer", UintegerValue ((numNode / 2) + 1)); // numNode starts at 0, so even nodes are the first half of the pair (AlexK.)
        }
      else
        {
          m_claFactory.Set ("RemotePeer", UintegerValue (base_addr)); // setting first node EngineId as remote peer (AlexK.)
          NS_LOG_FUNCTION(this << " Node number " << numNode << " out of " << nodeCnt << " is being assigned RemotePeer " << base_addr);
          //m_claFactory.Set ("RemotePeer", UintegerValue (numNode / 2)); // odd nodes are the second half of the pair (AlexK.)
        }

      InstallAndLink (*i);
      
    }

}
void
LtpProtocolHelper::InstallAndLink (Ptr<Node> n)
{
  NS_LOG_FUNCTION (this);
  Ptr<LtpConvergenceLayerAdapter> link = m_claFactory.Create ()->GetObject<LtpConvergenceLayerAdapter> ();
  if (link == 0)
    {
      NS_FATAL_ERROR ("The requested cla does not exist: \"" <<
                      m_claFactory.GetTypeId ().GetName () << "\"");
    }

  if (n->GetObject<LtpProtocol> () == 0)
  {
    Install (n);
  }
  Ptr<LtpProtocol> ltpProtocol = n->GetObject<LtpProtocol> ();

  link->SetProtocol (ltpProtocol);
  ltpProtocol->AddConvergenceLayerAdapter (link);
  ltpProtocol->SetIpResolutionTable (m_resolutionTable);
  link->EnableReceive (ltpProtocol->GetLocalEngineId ());

  ltpProtocol->EnableLinkStateCues (link);

  EnumValue mode = 0;
  m_resolutionTable->GetAttribute ("Addressing",mode);

  UintegerValue port = 0;
  link->GetAttribute ("ServerPort",port);

  if (mode.Get () == LtpIpResolutionTable::Ipv4)
    {
      Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
      //for (uint32_t i = 1; i < ipv4->GetNInterfaces (); i++) // 0 is loopback; add bindings for addresses on all of the nodes interfaces (AlexK.)
      //{
      //  m_resolutionTable->AddBinding (m_ltpid,ipv4->GetAddress (i,0).GetLocal (),port.Get ());
      //}
      m_resolutionTable->AddBinding (m_ltpid,ipv4->GetAddress (1,0).GetLocal (),port.Get ());
    }
  else
    {
      Ptr<Ipv6> ipv6 = n->GetObject<Ipv6> ();
      m_resolutionTable->AddBinding (m_ltpid,ipv6->GetAddress (1,0).GetAddress (),port.Get ());
    }

  m_ltpid++;
  link->SetLinkUpCallback ( MakeCallback (&LtpProtocol::Send,ltpProtocol));

  Simulator::Schedule (m_startTime, &LtpUdpConvergenceLayerAdapter::SetLinkUp, link);

}

/*
void
LtpProtocolHelper::LinkP2P (Ipv4InterfaceContainer iContainer)
{
  NS_LOG_FUNCTION (this);

  Ptr<Ipv4Interface> i0 = iContainer.Get (0);
  Ptr<Ipv3PointToPointInterfaceData> i0p2pData = i0->GetPointToPointData ();
  if (!i0p2pData)
  {
    NS_FATAL_ERROR ("LtpProtocolHelper::LinkP2P (): "
                    "Interface 0 is not a point-to-point interface");
                    return;
  }
  Ptr<Ipv4Interface> i1 = iContainer.Get (1);
  Ptr<Node> n0 = i0->GetNode ();
  Ptr<Node> n0 = i1->GetNode ();

  Ptr<LtpProtocol> ltpProtocol0 = n0->GetObject<LtpProtocol> ();
  Ptr<LtpProtocol> ltpProtocol1 = n1->GetObject<LtpProtocol> ();
  uint64_t ltpEngineId0 = ltpProtocol0->GetLocalEngineId ();
  uint64_t ltpEngineId1 = ltpProtocol1->GetLocalEngineId ();
  Link (netDev0, ltpEngineId1);
  Link (netDev1, ltpEngineId0);
}
*/

/*
void
LtpProtocolHelper::Link (Ptr<Ipv4Interface> interface, uint64_t remotePeer)
{
  NS_LOG_FUNCTION (this);

  m_claFactory.Set ("RemotePeer", UintegerValue (remotePeer));
  Ptr<LtpConvergenceLayerAdapter> link = m_claFactory.Create ()->GetObject<LtpConvergenceLayerAdapter> ();
  if (link == 0)
  {
    NS_FATAL_ERROR ("The requested cla does not exist: \"" <<
                    m_claFactory.GetTypeId ().GetName () << "\"");
  }

  Ptr<Node> n = interface->GetNode ();
  Ptr<LtpProtocol> ltpProtocol = n->GetObject<LtpProtocol> ();
  Ipv4InterfaceAccess iAddr = interface->GetAddress (0);
  Ipv4Address ipAddress = iAddr.GetLocal ();
  link->SetProtocol (ltpProtocol);
  link->SetRoutingProtocol (m_resolutionTable);
  ltpProtocol->AddConvergenceLayerAdapter (link);
  link->EnableReceive (ltpProtocol->GetLocalEngineId ());

  ltpProtocol->EnableLinkStateCues (link);
  uint64_t ltpEngineId = ltpProtocol->GetLocalEngineId ();

  UintegerValue port = 0;
  link->GetAttribute ("ServerPort",port);

  m_resolutionTable->AddBinding (ltpEngineId,ipAddress,port.Get ());

  //InstallAndLink (*i);
}
*/

void
LtpProtocolHelper::InstallAndLink (Ptr<Node> n, uint64_t remotePeer)
{
  NS_LOG_FUNCTION (this);
  m_claFactory.Set ("RemotePeer", UintegerValue (remotePeer));
  Ptr<LtpConvergenceLayerAdapter> link = m_claFactory.Create ()->GetObject<LtpConvergenceLayerAdapter> ();
  if (link == 0)
    {
      NS_FATAL_ERROR ("The requested cla does not exist: \"" <<
                      m_claFactory.GetTypeId ().GetName () << "\"");
    }

  Install (n);
  Ptr<LtpProtocol> ltpProtocol = n->GetObject<LtpProtocol> ();

  link->SetProtocol (ltpProtocol);
  ltpProtocol->AddConvergenceLayerAdapter (link);
  ltpProtocol->SetIpResolutionTable (m_resolutionTable);
  link->EnableReceive (ltpProtocol->GetLocalEngineId ());

  ltpProtocol->EnableLinkStateCues (link);

  EnumValue mode = 0;
  m_resolutionTable->GetAttribute ("Addressing",mode);

  UintegerValue port = 0;
  link->GetAttribute ("ServerPort",port);

  if (mode.Get () == LtpIpResolutionTable::Ipv4)
    {
      Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
      m_resolutionTable->AddBinding (m_ltpid,ipv4->GetAddress (1,0).GetLocal (),port.Get ());
    }
  else
    {
      Ptr<Ipv6> ipv6 = n->GetObject<Ipv6> ();
      m_resolutionTable->AddBinding (m_ltpid,ipv6->GetAddress (1,0).GetAddress (),port.Get ());
    }

  link->SetLinkUpCallback ( MakeCallback (&LtpProtocol::Send,ltpProtocol));

  Simulator::Schedule (m_startTime, &LtpUdpConvergenceLayerAdapter::SetLinkUp, link);

}



void
LtpProtocolHelper::Install (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  if (node->GetObject<LtpProtocol> () != 0)
    {
      NS_FATAL_ERROR ("LtpProtocolHelper::Install (): Aggregating "
                      "an LtpProtocol to a node with an existing Ltp object");
      return;
    }

  Ptr<LtpProtocol> ltpProtocol =  m_ltpFactory.Create ()->GetObject<LtpProtocol> ();
  ltpProtocol->SetAttribute ("LocalEngineId", UintegerValue (m_ltpid));
  ltpProtocol->SetNode (node);


  if (m_resolutionTable == 0)
    {
      NS_FATAL_ERROR ("LtpProtocolHelper::Install (): "
                      "Routing protocol is not set");
    }

  node->AggregateObject (ltpProtocol);

}
void
LtpProtocolHelper::Install (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  uint64_t base_addr = m_ltpid;
  uint32_t numNode = 0;

  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      if (++numNode < c.GetN ())
        {
          m_claFactory.Set ("RemotePeer", UintegerValue (m_ltpid + 1));
        }
      else
        {
          m_claFactory.Set ("RemotePeer", UintegerValue (base_addr));
        }

      Install (*i);
    }
}

void
LtpProtocolHelper::Install (std::string nodeName)
{
  NS_LOG_FUNCTION (this);
  Ptr<Node> node = Names::Find<Node> (nodeName);
  Install (node);
}

void
LtpProtocolHelper::Link (Ptr<Node> n, uint64_t remotePeer)
{
  NS_LOG_FUNCTION (this);
  m_claFactory.Set ("RemotePeer", UintegerValue (remotePeer));
  Ptr<LtpConvergenceLayerAdapter> link = m_claFactory.Create ()->GetObject<LtpConvergenceLayerAdapter> ();
  if (link == 0)
  {
    NS_FATAL_ERROR ("The requested cla does not exist: \"" <<
                    m_claFactory.GetTypeId ().GetName () << "\"");
  }

  Ptr<LtpProtocol> ltpProtocol = n->GetObject<LtpProtocol> ();
  link->SetProtocol (ltpProtocol);
  link->SetRoutingProtocol (m_resolutionTable);
  ltpProtocol->AddConvergenceLayerAdapter (link);
  link->EnableReceive (ltpProtocol->GetLocalEngineId ());

  ltpProtocol->EnableLinkStateCues (link);

  EnumValue mode = 0;
  m_resolutionTable->GetAttribute ("Addressing",mode);

  UintegerValue port = 0;
  link->GetAttribute ("ServerPort",port);

  if (mode.Get () == LtpIpResolutionTable::Ipv4)
    {
      Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
      m_resolutionTable->AddBinding (m_ltpid,ipv4->GetAddress (1,0).GetLocal (),port.Get ());
    }
  else
    {
      Ptr<Ipv6> ipv6 = n->GetObject<Ipv6> ();
      m_resolutionTable->AddBinding (m_ltpid,ipv6->GetAddress (1,0).GetAddress (),port.Get ());
    }

  link->SetLinkUpCallback ( MakeCallback (&LtpProtocol::Send,ltpProtocol));
  NS_LOG_FUNCTION (this << "Created link " << link << " with remote peer " << link->GetRemoteEngineId () << " on node " << n->GetId () << " with local engine id " << ltpProtocol->GetLocalEngineId ());
  Simulator::Schedule (m_startTime, &LtpUdpConvergenceLayerAdapter::SetLinkUp, link);
}

void
LtpProtocolHelper::SetBaseLtpEngineId (uint64_t id)
{
  NS_LOG_FUNCTION (this << id);
  m_ltpid = id;
}

void
LtpProtocolHelper::SetLtpEngineId (Ptr<Node> node, uint64_t id)
{
  NS_LOG_FUNCTION (this << node << id);
  Ptr<LtpProtocol> ltpProtocol = node->GetObject<LtpProtocol> ();
  ltpProtocol->SetAttribute ("LocalEngineId", UintegerValue (id));
}


void
LtpProtocolHelper::SetLtpIpResolutionTable (Ptr<LtpIpResolutionTable> rprot)
{
  NS_LOG_FUNCTION (this << rprot);
  m_resolutionTable = rprot;
}

bool
LtpProtocolHelper::AddBinding (uint64_t dstLtpEngineId, Ipv4Address dstAddr, uint16_t port)
{
  NS_LOG_FUNCTION (this << dstLtpEngineId << dstAddr << port);
  return m_resolutionTable->AddBinding (dstLtpEngineId,dstAddr,port);
}

bool
LtpProtocolHelper::AddBinding (uint64_t dstLtpEngineId, Ipv4Address dstAddr)
{
  NS_LOG_FUNCTION (this << dstLtpEngineId << dstAddr);
  return AddBinding (dstLtpEngineId,dstAddr,0);
}

void
LtpProtocolHelper::SetStartTransmissionTime (Time start)
{
  NS_LOG_FUNCTION (this << start);
  m_startTime = start;
}

} //namespace ltp
} //namespace ns3

