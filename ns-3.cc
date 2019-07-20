#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("My_Test_Script");

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
   	{
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

int main (int argc, char *argv[])
{
	bool verbose = true;
	// int choiceOfNetDevice = 0;
	uint32_t numberOfNodes = 4;
	int clientNode = 0;
	std::string chooseNetDevice = "";

	CommandLine cmd;
	cmd.AddValue ("chooseNetDevice","1-point-to-point\n\t\t\t2-csma\n\t\t\t3-wifi",chooseNetDevice);
	cmd.AddValue ("numberOfNodes", "Number of nodes/devices in the network", numberOfNodes);
	cmd.AddValue ("clientNode", "The Client Node", clientNode);
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
	cmd.Parse (argc,argv);

	Time::SetResolution (Time::NS);
	  
	if (verbose){
		LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
	}

	if (numberOfNodes > 7){
      std::cout << "Number of Nodes must be 7 or less; otherwise grid layout for Wifi Net Device and Point-To-Point Net Device becomes too complex" << std::endl;
      return 1;
    }

    clientNode = (clientNode == 0) ? 0 : clientNode-1;

    if(chooseNetDevice == ""){

    	NS_LOG_UNCOND("Choose Net Device from command line");

    } else if (chooseNetDevice == "point-to-point"){

    	/*
			Default Network Topology
		   Point-to-Point connections
		   n0------n1------n2------n3
			10.1.1.0
					10.1.2.0	
							10.1.3.0 	  
		*/

    	NS_LOG_UNCOND("Point-To-Point network");

		NodeContainer nodes;
		nodes.Create (numberOfNodes);

		std::string IP[numberOfNodes];
		for(uint32_t i=0;i<numberOfNodes-1;i++){
			IP[i] = "10.1."+std::to_string(i+1)+".0";
		}
		
		InternetStackHelper stack;
		stack.Install (nodes);
		
		PointToPointHelper pointToPoint;
		pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
		pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

		Ipv4AddressHelper address;
		NetDeviceContainer devices;
		Ipv4InterfaceContainer interfaces;

		for(uint32_t i=0;i<numberOfNodes-1;i++){
			address.SetBase (ns3::Ipv4Address(IP[i].c_str()),"255.255.255.0");
			devices = pointToPoint.Install(nodes.Get(i),nodes.Get(i+1));
			interfaces = address.Assign(devices);
		}
		
		Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	  
		UdpEchoServerHelper echoServer (9);

		ApplicationContainer serverApps = echoServer.Install (nodes.Get (numberOfNodes-1));
		serverApps.Start (Seconds (1.0));
		serverApps.Stop (Seconds (10.0));

		UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
		echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
		echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
		echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

		ApplicationContainer clientApps = echoClient.Install (nodes.Get (clientNode));
		clientApps.Start (Seconds (2.0));
		clientApps.Stop (Seconds (10.0));

		// AnimationInterface anim ("anim1.xml");
		// for(uint32_t i=0;i<numberOfNodes;i++){
		// 	anim.SetConstantPosition (nodes.Get(i), (double) i, (double) i);
		// }

		pointToPoint.EnablePcapAll ("point-to-point");		
	
    } else if (chooseNetDevice == "csma") {

		/*
			Default Network Topology
				CSMA channel
			n0    n1    n2    n3
			|     |     |      |
			====================
				  Ethernet
				 10.1.1.0
		*/

    	NS_LOG_UNCOND("CSMA channel");

		//Creating a container for CSMA nodes
		NodeContainer csmaNodes;
		csmaNodes.Create(numberOfNodes);

		//Establishing channel attributes 
		CsmaHelper csma;
		csma.SetChannelAttribute("DataRate",StringValue("10000Mbps"));
		csma.SetChannelAttribute("Delay",StringValue("1ms"));

		//Installing the nodes
		NetDeviceContainer csmaDevices;
		csmaDevices = csma.Install(csmaNodes);

		//Installing internet stack for the installed nodes
		InternetStackHelper stack;
		stack.Install(csmaNodes);

		//Setting base address for communication
		Ipv4AddressHelper address;
		address.SetBase("10.1.1.0","255.255.255.0");
	  
		Ipv4InterfaceContainer csmaInterfaces;
		csmaInterfaces = address.Assign(csmaDevices);

		UdpEchoServerHelper echoServer (19);

		//Server application 
		ApplicationContainer serverApps = echoServer.Install(csmaNodes.Get(numberOfNodes-1));
		serverApps.Start(Seconds (1.0));
		serverApps.Stop(Seconds (10.0));

		UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress(numberOfNodes-1),19);
		echoClient.SetAttribute("MaxPackets",UintegerValue(1));
		echoClient.SetAttribute("Interval",TimeValue(Seconds (1.0)));
		echoClient.SetAttribute("PacketSize",UintegerValue(512));

		//Client Application
		ApplicationContainer clientApps = echoClient.Install(csmaNodes.Get(clientNode));
		clientApps.Start(Seconds (2.0));
		clientApps.Stop(Seconds (10.0));

		//Populating Routing tables for routing the packets
		Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

		//enabling pcap files for debugging
		csma.EnablePcap ("second", csmaDevices.Get (clientNode), true);

    } else if (chooseNetDevice == "wifi") {

		// This program configures a wifi adhoc network of nodes (default = 4) on an
		// 802.11b physical layer, with
		// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
		// (application) bytes to node 1.

		//  *    *    *    *
		//  |    |    |    |    
		// n0   n1   n2   n3 

    	NS_LOG_UNCOND("Wifi adhoc network");

		std::string phyMode ("DsssRate1Mbps");
		double distance = 500;  // m
		uint32_t packetSize = 1000; // bytes
		uint32_t numPackets = 1;
		uint32_t numNodes = numberOfNodes;  
		uint32_t sinkNode = clientNode;
		uint32_t sourceNode = numberOfNodes-1;
		double interval = 1.0; // seconds
		bool verbose = false;
		bool tracing = false;
		Time interPacketInterval = Seconds (interval);

		// Fix non-unicast data rate to be the same as that of unicast
		Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
		                      StringValue (phyMode));

		NodeContainer c;
		c.Create (numNodes);

		// The below set of helpers will help us to put together the wifi NICs we want
		WifiHelper wifi;
		if (verbose){
			wifi.EnableLogComponents ();  // Turn on all Wifi logging
		}

		YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
		// set it to zero; otherwise, gain will be added
		wifiPhy.Set ("RxGain", DoubleValue (-10) );
		// ns-3 supports RadioTap and Prism tracing extensions for 802.11b
		wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

		YansWifiChannelHelper wifiChannel;
		wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
		wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
		wifiPhy.SetChannel (wifiChannel.Create ());

		// Add an upper mac and disable rate control
		WifiMacHelper wifiMac;
		wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
		wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
		                                "DataMode",StringValue (phyMode),
		                                "ControlMode",StringValue (phyMode));
		// Set it to adhoc mode
		wifiMac.SetType ("ns3::AdhocWifiMac");
		NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

		MobilityHelper mobility;
		mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		                                 "MinX", DoubleValue (0.0),
		                                 "MinY", DoubleValue (0.0),
		                                 "DeltaX", DoubleValue (distance),
		                                 "DeltaY", DoubleValue (distance),
		                                 "GridWidth", UintegerValue (5),
		                                 "LayoutType", StringValue ("RowFirst"));
		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
		mobility.Install (c);

		// Enable OLSR
		OlsrHelper olsr;
		Ipv4StaticRoutingHelper staticRouting;

		Ipv4ListRoutingHelper list;
		list.Add (staticRouting, 0);
		list.Add (olsr, 10);

		InternetStackHelper internet;
		internet.SetRoutingHelper (list); // has effect on the next Install ()
		internet.Install (c);

		Ipv4AddressHelper ipv4;
		NS_LOG_INFO ("Assign IP Addresses.");
		ipv4.SetBase ("10.1.1.0", "255.255.255.0");
		Ipv4InterfaceContainer i = ipv4.Assign (devices);

		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (sinkNode), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
		recvSink->Bind (local);
		recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

		Ptr<Socket> source = Socket::CreateSocket (c.Get (sourceNode), tid);
		InetSocketAddress remote = InetSocketAddress (i.GetAddress (sinkNode, 0), 80);
		source->Connect (remote);

		if (tracing == true){
			AsciiTraceHelper ascii;
		    wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
		    wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);
		    // Trace routing tables
		    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);
		    olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
		    Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.neighbors", std::ios::out);
		    olsr.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

		    // To do-- enable an IP-level trace that shows forwarding events only
		}

		// Give OLSR time to converge-- 30 seconds perhaps
		Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
		                       source, packetSize, numPackets, interPacketInterval);
		
		// AnimationInterface anim ("anim5.xml");
		NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);
		
		Simulator::Stop (Seconds (33.0));
    }

	Simulator::Run ();
	Simulator::Destroy ();
	return 0;
}