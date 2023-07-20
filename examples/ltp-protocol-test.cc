/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*-
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

//        Network topology
//
//       n0              n1
//       |               |
//       =================
//          PointToPoint
//
// - Send a block of data from one service instance in node n0 to the other in node n1.
// - Data is sent end-to-end through a LtpProtcol <-> UdpLayerAdapter <-> PointToPointLink <-> UdpLayerAdapter <-> LtpProtcol.
// - Functions (ClientServiceInstanceNotificationsSnd and ClientServiceInstanceNotificationsRcv) are used for tracing
//

#include "ns3/core-module.h"
#include "ns3/ltp-protocol-helper.h"
#include "ns3/ltp-protocol.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include <sstream>

#include "json.hpp"
using json = nlohmann::json;

using namespace ns3;
using namespace ltp;

NS_LOG_COMPONENT_DEFINE ("LtpProtocolSimpleExample");

std::vector<uint8_t> receivedGreenData;
std::vector<uint8_t> receivedRedData;
uint32_t totalReceived = 0;
uint32_t totalSent = 0;
uint32_t totalSending = 0;

struct TxMapVals {
        ns3::ltp::SessionId sessionId;
        uint64_t dstLtpEngineId;
        bool set;
        uint32_t redSize;
        uint32_t bundleNumber;
        NodeContainer nodes;
    };

std::map<json, TxMapVals> m_txSessionMap;

void
sendFromQueue ()
{
    NS_LOG_INFO ("::sendFromQueue:: start: " << Simulator::Now ().GetMilliSeconds ());
    if (m_txSessionMap.size () > 0)
    {
        std::map<json, TxMapVals>::iterator it = m_txSessionMap.begin ();

        std::vector<uint8_t> data1 = json::to_cbor (it->first);
        uint64_t redSize = it->second.redSize;
        uint64_t receiverLtpId = it->second.dstLtpEngineId;
        NS_LOG_INFO ("::sendFromQueue:: Sending bundle number: " << it->second.bundleNumber);
        NS_LOG_INFO ("::sendFromQueue:: Data size: " << data1.size () << " Red Data size: " << redSize);
        NS_LOG_INFO ("::sendFromQueue:: Calling StartTransmission");
        uint32_t ClientServiceId = 1;
        it->second.nodes.Get (0)->GetObject<LtpProtocol> ()->StartTransmission (ClientServiceId,ClientServiceId,receiverLtpId,data1,redSize);
        totalSent ++;
    }
}

void
addToSendQueue (NodeContainer nodes, json jsonData, uint64_t receiverLtpId, uint64_t redSize)
{
    NS_LOG_INFO ("::addToSendQueue:: start: " << Simulator::Now ().GetMilliSeconds ());
    // build the TxVals struct
    TxMapVals txMapVals;
    txMapVals.bundleNumber = totalSending;
    txMapVals.redSize = redSize;
    txMapVals.dstLtpEngineId = receiverLtpId;
    txMapVals.set = false;
    txMapVals.nodes = nodes;

    // add the jsonData and TxVals to the map
    std::map<json, TxMapVals>::iterator it = m_txSessionMap.find(jsonData);
    if (it == m_txSessionMap.end ())
    {
        m_txSessionMap.insert(std::pair<json, TxMapVals>(jsonData, txMapVals));
        NS_LOG_INFO ("::addToSendQueue:: jsonData added to map");
    }
    else
    {
        NS_LOG_INFO ("::addToSendQueue:: ERROR - jsonData already in map");
    }
    totalSending++;
    
    if (m_txSessionMap.size() == 1)
    {
        NS_LOG_INFO ("::addToSendQueue:: calling sendFromQueue");
        //sendFromQueue ();
        Simulator::ScheduleNow (&sendFromQueue);
    }
    NS_LOG_INFO ("::addToSendQueue:: done");
}

void
charArrayToVec (const char* charArray, std::vector<uint8_t> &vec)
{
  size_t arrayLen = strlen(charArray);

  vec.resize(arrayLen);

  for (size_t i = 0; i < arrayLen; i++)
    {
      vec.push_back (static_cast<uint8_t>(charArray[i]));
    }
}

void
ClientServiceInstanceNotificationsSnd (SessionId id,
                                       StatusNotificationCode code,
                                       std::vector<uint8_t> data,
                                       uint32_t dataLength,
                                       bool endFlag,
                                       uint64_t srcLtpEngine,
                                       uint32_t offset )
{
  NS_LOG_INFO (Simulator::Now ().GetMilliSeconds () << "SENDER ClientServiceNotification - Session ID: " << id.GetSessionNumber () << " Code : " << code);
  if (code == ns3::ltp::SESSION_START)
  {
    NS_LOG_INFO (Simulator::Now ().GetMilliSeconds () << "Sender ClientServiceNotification - Session " << id.GetSessionNumber () << " Started");
    if (m_txSessionMap.size () > 0)
    {
        std::map<json, TxMapVals>::iterator it = m_txSessionMap.begin ();
        for (; it != m_txSessionMap.end (); it++)
        {
            if (it->second.set == false)
            {
                it->second.sessionId = id;
                it->second.set = true;
                NS_LOG_INFO ("Sender ClientServiceNotification - Session " << id.GetSessionNumber () << " set in map");
                break;
            }
        }
        if (it == m_txSessionMap.end())
        {
            NS_LOG_INFO ("Sender ClientServiceNotification - ERROR - Session " << id.GetSessionNumber () << " not set in map");
        }
    }
  }
  if (code == ns3::ltp::SESSION_END)
  {
    NS_LOG_INFO ("Sender ClientServiceNotification - Session " << id.GetSessionNumber () << " Ended");
    if (m_txSessionMap.size () > 0)
    {
        std::map<json, TxMapVals>::iterator it = m_txSessionMap.begin ();
        for (; it != m_txSessionMap.end (); it++)
        {
            if (it->second.sessionId == id)
            {
                m_txSessionMap.erase(it);
                NS_LOG_INFO ("Sender ClientServiceNotification - Session " << id.GetSessionNumber () << " removed from map");
                break;
            }
        }
        if (it == m_txSessionMap.end())
        {
            NS_LOG_INFO ("Sender ClientServiceNotification - ERROR - Session " << id.GetSessionNumber () << " not found in map");
            return;
        }
    }
    else
    {
        NS_LOG_INFO ("Sender ClientServiceNotification - ERROR - Session " << id.GetSessionNumber () << " not found in map");
    }
    uint32_t quSize = m_txSessionMap.size ();
    if (quSize > 0)
    {
        NS_LOG_INFO ("Sender ClientServiceNotification - " << quSize << "Still bundles in the queue; Scheduling sendFromQueue");
        //sendFromQueue ();
        Simulator::ScheduleNow (&sendFromQueue);
    }
  }
}

void
ClientServiceInstanceNotificationsRcv (SessionId id,
                                       StatusNotificationCode code,
                                       std::vector<uint8_t> data,
                                       uint32_t dataLength,
                                       bool endFlag,
                                       uint64_t srcLtpEngine,
                                       uint32_t offset )
{
  static uint32_t totalData = 0;

  NS_LOG_INFO ("RECEIVER ClientServiceNotification - Session ID: " << id.GetSessionNumber () << " Code : " << code);

  totalData += dataLength;

  if (code == ns3::ltp::GP_SEGMENT_RCV)
    {

      NS_LOG_INFO ("ClientServiceNotification - Received a Green Data Segment of Size: ( " << dataLength << ")");
    
      for ( std::vector<uint8_t>::const_iterator i = data.begin (); i != data.end (); ++i)
      {
        receivedGreenData.push_back(*i);
      }
    }
  if (code == ns3::ltp::RED_PART_RCV)
    {
      std::stringstream ss;

      for ( std::vector<uint8_t>::const_iterator i = data.begin (); i != data.end (); ++i)
        {
          ss << *i;
          receivedRedData.push_back(*i);
        }

      NS_ASSERT (ss.str ().length () == dataLength);

      NS_LOG_INFO ("ClientServiceNotification - Received Full Red Part of Size: ( " << dataLength << ")");

    }
  if (code == ns3::ltp::SESSION_END)
    {
      NS_LOG_INFO ("ClientServiceNotification - Received a full block of data with size: ( " << totalData << ") from LtpEngine: " << srcLtpEngine);
      std::vector<uint8_t> receivedData;
      if (receivedRedData.size () > 0)
      {
        NS_LOG_INFO ("Total Red data received: " << receivedRedData.size ());
        for (std::vector<uint8_t>::const_iterator r_i = receivedRedData.begin (); r_i != receivedRedData.end (); ++r_i)
        {
          receivedData.push_back(*r_i);
        }
      }
      if (receivedGreenData.size () > 0)
      {
        NS_LOG_INFO ("Total Green data received: " << receivedGreenData.size ());
        for (std::vector<uint8_t>::const_iterator g_i = receivedGreenData.begin (); g_i != receivedGreenData.end (); ++g_i)
        {
          receivedData.push_back(*g_i);
        }
      }
      NS_LOG_INFO ("Total data received: " << receivedData.size ());
      json jsonData = json::from_cbor (receivedData);
      NS_LOG_INFO ("jsonData:");
      for (json::iterator it = jsonData.begin (); it != jsonData.end (); it++)
      {
        NS_LOG_INFO ("\t" << it.key() << " : " << it.value());
      }
      //std::cout << "jsonData['string'] = " << jsonData["string"] << std::endl;
      //Reset buffers and counters
      receivedRedData.clear ();
      receivedGreenData.clear ();
      totalData = 0;
      totalReceived ++;
      NS_LOG_INFO ("Total data bundles received: " << totalReceived);
      
    }

}

void send0 (NodeContainer nodes, std::vector<uint8_t> data, uint64_t redSize, uint64_t ClientServiceId, uint64_t receiverLtpId)
{
    NS_LOG_INFO ("::send0:: start: " << Simulator::Now ().GetMilliSeconds ());
    uint64_t size = data.size();
    NS_LOG_INFO ("::send0:: Data size: "<< size << " Red Data size: " << redSize);
    nodes.Get (0)->GetObject<LtpProtocol> ()->StartTransmission (ClientServiceId,ClientServiceId,receiverLtpId,data,redSize);
    totalSent ++;
    NS_LOG_INFO ("::send0:: done");
}

void send1 (NodeContainer nodes, uint64_t ClientServiceId, uint64_t receiverLtpId)
{
  NS_LOG_INFO ("::send1:: start: " << Simulator::Now ().GetMilliSeconds ());
  const char* dataBuffer = "send1: Books serve to show a man that those original thoughts of his aren't very new after all.";

  std::vector<uint8_t> data1;
  charArrayToVec(dataBuffer, data1);

  std::cout << "::send1:: data1.size() = " << data1.size() << std::endl;
  std::cout << "::send1:: data1.max_size() = " << data1.max_size() << std::endl;
  std::cout << "::send1:: data1.capacity() = " << data1.capacity() << std::endl;

  //uint64_t redSize = 0;
  uint64_t redSize = data1.size()/2;
  uint64_t size = data1.size();
  NS_LOG_INFO ("::send1:: Data size: "<< size << " Red Data size: " << redSize);
  nodes.Get (0)->GetObject<LtpProtocol> ()->StartTransmission (ClientServiceId,ClientServiceId,receiverLtpId,data1,redSize);
  totalSent ++;
  //std::vector<uint8_t>().swap(data1);
  NS_LOG_INFO ("::send1:: done");
}

void send2 (NodeContainer nodes, uint64_t ClientServiceId, uint64_t receiverLtpId)
{
  NS_LOG_INFO ("::send2:: start: " << Simulator::Now ().GetMilliSeconds ());
  const char* dataBuffer = "send2: Books serve to show a man that those original thoughts of his aren't very new after all.";

  json jsonData;
  jsonData ["sendingFunc"] = "send2";
  jsonData ["sendTime"] = std::to_string(Simulator::Now ().GetMilliSeconds ());
  jsonData ["string"] = dataBuffer;
  jsonData ["totalSent"] = totalSent;
  jsonData ["bundleNumber"] = totalSent + 1;
  std::vector<uint8_t> data1 = json::to_cbor (jsonData);

  NS_LOG_INFO ("::send2:: data1.size() = " << data1.size());
  NS_LOG_INFO ("::send2:: data1.max_size() = " << data1.max_size());
  NS_LOG_INFO ("::send2:: data1.capacity() = " << data1.capacity());

  //uint64_t redSize = 0;
  uint64_t redSize = data1.size()/2;
  uint64_t size = data1.size();
  NS_LOG_INFO ("::send2:: Data size: "<< size << " Red Data size: " << redSize);
  NS_LOG_INFO ("::send2:: Calling StartTransmission");
  nodes.Get (0)->GetObject<LtpProtocol> ()->StartTransmission (ClientServiceId,ClientServiceId,receiverLtpId,data1,redSize);
  totalSent ++;
  NS_LOG_INFO ("::send2:: Clearing cbor vector");
  std::vector<uint8_t>().swap(data1);
  NS_LOG_INFO ("::send2:: done");
}

void send3 (NodeContainer nodes, uint64_t ClientServiceId, uint64_t receiverLtpId)
{
  NS_LOG_INFO ("::send3:: start: " << Simulator::Now ().GetMilliSeconds ());
  const char* dataBuffer = "send3: Books serve to show a man that those original thoughts of his aren't very new after all.";
  
  size_t bufferSize = std::strlen(dataBuffer);
  char* heapBuffer = new char[bufferSize];
  std::strcpy(heapBuffer, dataBuffer);

  NS_LOG_INFO ("::send3:: heapBuffer = " << heapBuffer);
  
  json jsonData;
  jsonData ["sendingFunc"] = "send3";
  jsonData ["sendTime"] = std::to_string(Simulator::Now ().GetMilliSeconds ());
  jsonData ["string"] = heapBuffer;
  std::vector<uint8_t> data1 = json::to_cbor (jsonData);
  
  NS_LOG_INFO ("::send3:: data1.size() = " << data1.size());
  NS_LOG_INFO ("::send3:: data1.max_size() = " << data1.max_size());
  NS_LOG_INFO ("::send3:: data1.capacity() = " << data1.capacity());

  uint64_t redSize = 0;
  //uint64_t redSize = data1.size()/2;
  uint64_t size = data1.size();
  NS_LOG_INFO ("::send3:: Data size: "<< size << " Red Data size: " << redSize);
  NS_LOG_INFO ("::send3:: Calling StartTransmission");
  nodes.Get (0)->GetObject<LtpProtocol> ()->StartTransmission (ClientServiceId,ClientServiceId,receiverLtpId,data1,redSize);
  totalSent ++;
  NS_LOG_INFO ("::send3:: Freeing heapBuffer");
  delete[] heapBuffer;
  NS_LOG_INFO ("::send3:: done");
}

void
send4 (NodeContainer nodes, uint64_t ClientServiceId, uint64_t receiverLtpId)
{
    NS_LOG_INFO ("::send4:: start: " << Simulator::Now ().GetMilliSeconds ());
    const char* dataBuffer = "send4: Books serve to show a man that those original thoughts of his aren't very new after all.";

    json jsonData;
    jsonData ["sendingFunc"] = "send4";
    jsonData ["sendTime"] = std::to_string(Simulator::Now ().GetMilliSeconds ());
    jsonData ["string"] = dataBuffer;
    jsonData ["totalSent"] = totalSent;
    jsonData ["bundleNumber"] = totalSending;

    uint64_t redSize = std::strlen(dataBuffer)/2;

    NS_LOG_INFO ("::send4:: calling addToSendQueue()");
    addToSendQueue (nodes, jsonData, receiverLtpId, redSize);
    
    NS_LOG_INFO ("::send4:: done");
}

int
main (int argc, char *argv[])
{
  bool verbose = true;
  bool tracing = true;

  if (verbose)
    {
      LogComponentEnable ("LtpProtocolSimpleExample", LOG_LEVEL_ALL);
      //LogComponentEnable ("LtpProtocol", LOG_LEVEL_ALL);
    }

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("tracing", "Tell application to generate trace files if true", tracing);
  cmd.Parse (argc,argv);

  // Create the nodes required by the topology (shown above).
  NodeContainer nodes;
  nodes.Create (2);
  
  std::ostringstream channelDelay;
  //channelDelay << "750s"; // Earth to Mars - Average
  channelDelay << "1ms";

  // Create point to point links and instell them on the nodes
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (channelDelay.str ()));

  if (tracing)
    {
      NS_LOG_INFO ("Configure Tracing.");
      AsciiTraceHelper ascii;
      pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("ltp-protocol-example.tr"));
      pointToPoint.EnablePcapAll ("ltp-protocol-example");
    }

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  // Install the internet stack on the nodes
  InternetStackHelper internet;
  internet.Install (nodes);

  // Assign IPv4 addresses.
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  // Defines the ClientService ID code that will be using the Ltp protocol in both sides of the link
  // Bundle protocol code as defined by IANA: "LTP Client Service Identifiers" referenced in RFC 7116
  uint64_t ClientServiceId = 1;

  // Creta a LtpIpResolution table to perform mappings between Ipv4 adresses and LtpEngineIDs
  Ptr<LtpIpResolutionTable> routing =  CreateObjectWithAttributes<LtpIpResolutionTable> ("Addressing", StringValue ("Ipv4"));

  // Use a helper to create and install Ltp Protocol instances in the nodes.
  LtpProtocolHelper ltpHelper;
  ltpHelper.SetAttributes ("CheckPointRtxLimit",  UintegerValue (20),
                           "ReportSegmentRtxLimit", UintegerValue (20),
                           "RetransCyclelimit",  UintegerValue (20),
                           "OneWayLightTime", StringValue (channelDelay.str ()));
  ltpHelper.SetLtpIpResolutionTable (routing);
  ltpHelper.SetBaseLtpEngineId (0);
  //ltpHelper.SetStartTransmissionTime (Seconds (1));
  ltpHelper.InstallAndLink (nodes);

  // Register ClientServiceInstances with the corresponding Ltp Engine of each node
  CallbackBase cb = MakeCallback (&ClientServiceInstanceNotificationsSnd);
  CallbackBase cb2 = MakeCallback (&ClientServiceInstanceNotificationsRcv);
  nodes.Get (0)->GetObject<LtpProtocol> ()->RegisterClientService (ClientServiceId,cb);
  nodes.Get (1)->GetObject<LtpProtocol> ()->RegisterClientService (ClientServiceId,cb2);

  // Create a block of data
  std::vector<uint8_t> data3 ( 5000, 65);
  const char* dataBuffer = "Main: Books serve to show a man that those original thoughts of his aren't very new after all.";
    
  std::vector<uint8_t> data1;
  charArrayToVec(dataBuffer, data1);
  //std::cout << "data1.size() = " << data1.size() << std::endl;
  //std::cout << "data1.max_size() = " << data1.max_size() << std::endl;
  //std::cout << "data1.capacity() = " << data1.capacity() << std::endl;


  
  json jsonData1;
  jsonData1 ["string"] = dataBuffer;
  std::vector<uint8_t> data2 = json::to_cbor (jsonData1);
  

  // Transmit if from n0 to n1.
  uint64_t receiverLtpId = nodes.Get (1)->GetObject<LtpProtocol> ()->GetLocalEngineId ();
  //nodes.Get (0)->GetObject<LtpProtocol> ()->StartTransmission (ClientServiceId,ClientServiceId,receiverLtpId,data3,1500);
  //nodes.Get (0)->GetObject<LtpProtocol> ()->StartTransmission (ClientServiceId,ClientServiceId,receiverLtpId,data2,data2.size()/2);
  
  //send(nodes, ClientServiceId, receiverLtpId);
  //Simulator::Schedule (Seconds (1), &send0, nodes, data2, data2.size()/2, ClientServiceId, receiverLtpId);
  Simulator::Schedule (Seconds (1), &send4, nodes, ClientServiceId, receiverLtpId);
  Simulator::Schedule (Seconds (1), &send4, nodes, ClientServiceId, receiverLtpId);
  Simulator::Schedule (Seconds (2), &send4, nodes, ClientServiceId, receiverLtpId);
  Simulator::Schedule (Seconds (2), &send4, nodes, ClientServiceId, receiverLtpId);
  Simulator::Schedule (Seconds (2), &send4, nodes, ClientServiceId, receiverLtpId);
  //Simulator::Schedule (Seconds (1.01), &send2, nodes, ClientServiceId, receiverLtpId);
  //Simulator::Schedule (Seconds (1.01), &send2, nodes, ClientServiceId, receiverLtpId);
  
  //Simulator::Schedule (Seconds (1 + std::stoi (channelDelay.str ())), &send2, nodes, ClientServiceId, receiverLtpId);
  //Simulator::Schedule (Seconds (1 + std::stoi (channelDelay.str ()) + std::stoi (channelDelay.str ())), &send3, nodes, ClientServiceId, receiverLtpId);

  

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}



