/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 NITK Surathkal
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
 * Authors: Vivek Jain <jain.vivek.anand@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "sfb-queue-disc.h"
#include "ns3/ipv4-packet-filter.h"
#include "ns3/ipv6-packet-filter.h"
#include "ns3/drop-tail-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SfbQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (SfbQueueDisc);

TypeId SfbQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SfbQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<SfbQueueDisc> ()
    .AddAttribute ("Mode",
                   "Determines unit for QueueLimit",
                   EnumValue (QUEUE_DISC_MODE_BYTES),
                   MakeEnumAccessor (&SfbQueueDisc::SetMode),
                   MakeEnumChecker (QUEUE_DISC_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    QUEUE_DISC_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("QueueLimit",
                   "Queue limit in bytes/packets",
                   UintegerValue (25),
                   MakeUintegerAccessor (&SfbQueueDisc::SetQueueLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MeanPktSize",
                   "Average of packet size",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&SfbQueueDisc::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Increment",
                   "Pmark increment value",
                   DoubleValue (0.0025),
                   MakeDoubleAccessor (&SfbQueueDisc::m_increment),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Decrement",
                   "Pmark decrement Value",
                   DoubleValue (0.00025),
                   MakeDoubleAccessor (&SfbQueueDisc::m_decrement),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RehashInterval",
                   "Time interval after which hash will be changed",
                   TimeValue (Seconds (600)),
                   MakeTimeAccessor (&SfbQueueDisc::m_rehashInterval),
                   MakeTimeChecker ())
    .AddAttribute ("PenaltyRate",
                   "Penalty rate value packet per second",
                   UintegerValue (10),
                   MakeUintegerAccessor (&SfbQueueDisc::m_penaltyRate),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("PenaltyBurst",
                   "Penalty burst value ",
                   UintegerValue (20),
                   MakeUintegerAccessor (&SfbQueueDisc::m_penaltyBurst),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TokenAvail",
                   "Token available",
                   UintegerValue (20),
                   MakeUintegerAccessor (&SfbQueueDisc::m_tokenAvail),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TargetFraction",
                   "target bucket length",
                   DoubleValue (0.75),
                   MakeDoubleAccessor (&SfbQueueDisc::m_targetFraction),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

SfbQueueDisc::SfbQueueDisc ()
  : QueueDisc ()
{
  NS_LOG_FUNCTION (this);
  m_uv = CreateObject<UniformRandomVariable> ();
  m_pertbUV = CreateObject<UniformRandomVariable> ();
}

SfbQueueDisc::~SfbQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
SfbQueueDisc::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_uv = 0;
  QueueDisc::DoDispose ();
}

void
SfbQueueDisc::SetMode (QueueDiscMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

SfbQueueDisc::QueueDiscMode
SfbQueueDisc::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}

void
SfbQueueDisc::SetQueueLimit (uint32_t lim)
{
  NS_LOG_FUNCTION (this << lim);
  m_queueLimit = lim;
}

uint32_t
SfbQueueDisc::GetQueueSize (void)
{
  NS_LOG_FUNCTION (this);
  if (GetMode () == QUEUE_DISC_MODE_BYTES)
    {
      return GetInternalQueue (0)->GetNBytes ();
    }
  else if (GetMode () == QUEUE_DISC_MODE_PACKETS)
    {
      return GetInternalQueue (0)->GetNPackets ();
    }
  else
    {
      NS_ABORT_MSG ("Unknown SFB mode.");
    }
}

int64_t
SfbQueueDisc::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uv->SetStream (stream);
  m_pertbUV->SetStream (stream + 1);
  return 1;
}

void
SfbQueueDisc::InitializeParams (void)
{
  InitializeBins ();
  m_binSize = 1.0 / SFB_BINS * GetQueueSize ();
  m_target = m_binSize * m_targetFraction;
  m_lastSet = Simulator::Now ();
}

void
SfbQueueDisc::InitializeBins (void)
{
  m_bins.perturbation = m_pertbUV->GetInteger ();
  m_lastSet = Simulator::Now ();
  for (uint32_t i = 0; i < SFB_LEVELS; i++)
    {
      for (uint32_t j = 0; j < SFB_BINS; j++)
        {
          m_bins.bins[i][j].packets = 0;
          m_bins.bins[i][j].pmark = 0;
        }
    }
}

uint32_t
SfbQueueDisc::SfbHash (Ptr<QueueDiscItem> item)
{
  uint32_t sfbhash = Classify (item);
  uint32_t buff[2];
  buff[0] = sfbhash;
  buff[1] = m_bins.perturbation;
  sfbhash = Hash32 ((char*) buff,2);
  return sfbhash;
}

bool
SfbQueueDisc::RateLimit (Ptr<QueueDiscItem> item)
{
  /*NS_LOG_FUNCTION (this);
  if (m_penaltyRate == 0 || m_penaltyBurst == 0)
    {
      return true;
    }

  if (m_tokenAvail < 1)
    {
      double value = std::min (10.0, Simulator::Now ().GetSeconds () - m_tokenTime.GetSeconds ());
      m_tokenAvail = value * m_penaltyRate;

      if (m_tokenAvail > m_penaltyBurst)
        {
          m_tokenAvail = m_penaltyBurst;
        }

      m_tokenTime = Simulator::Now ();
      if (m_tokenAvail < 1)
        {
          return true;
        }
    }

  m_tokenAvail--;
*/
  return false;
}

bool
SfbQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);
  uint32_t nQueued = GetQueueSize ();
  uint32_t minqlen = m_binSize;
  if ((GetMode () == QUEUE_DISC_MODE_PACKETS && nQueued >= m_queueLimit)
      || (GetMode () == QUEUE_DISC_MODE_BYTES && nQueued + item->GetSize () > m_queueLimit))
    {
      IncrementBinsPmarks (item);
      DropBeforeEnqueue (item,OVERLIMIT_DROP);
      return false;
    }

  if (m_rehashInterval > 0)
    {
      if (Simulator::Now () > (m_lastSet + m_rehashInterval))
        {
          m_bins.perturbation = m_pertbUV->GetInteger ();
          m_lastSet = Simulator::Now ();
        }
    }

  uint32_t sfbhash = SfbHash (item);
  for (uint32_t i = 0; i < SFB_LEVELS; i++)
    {
      uint32_t hashed = sfbhash & SFB_BUCKET_MASK;
      if (m_bins.bins[i][hashed].packets == 0)
        {
          DecrementBinPmark (i, hashed);
        }
      else if (m_bins.bins[i][hashed].packets > m_target)
        {
          IncrementBinPmark (i, hashed);
        }

      if (m_binSize > m_bins.bins[i][hashed].packets)
        {
          minqlen = m_bins.bins[i][hashed].packets;
        }

      sfbhash >>= SFB_BUCKET_SHIFT;
    }

  if (minqlen >= m_binSize)
    {
      DropBeforeEnqueue (item, BUCKETLIMIT_DROP);
      return false;
    }

  if (GetMinProbability (item) == 1.0)
    {
      if (RateLimit (item))
        {
          DropBeforeEnqueue (item, RATELIMIT_DROP);
          return false;
        }
    }
  else if (DropEarly (item))
    {
      DropBeforeEnqueue (item, TARGET_EXCED_DROP);
      return false;
    }

  bool isEnqueued = GetInternalQueue (0)->Enqueue (item);
  if (isEnqueued == true)
    {
      IncrementBinsQueueLength (item);
    }
  return isEnqueued;
}

double
SfbQueueDisc::GetMinProbability (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this);
  double u =  1.0;
  uint32_t sfbhash = SfbHash (item);
  for (uint32_t i = 0; i < SFB_LEVELS; i++)
    {
      uint32_t hashed = sfbhash & SFB_BUCKET_MASK;
      if (u > m_bins.bins[i][hashed].pmark)
        {
          u = m_bins.bins[i][hashed].pmark;
        }
      sfbhash >>= SFB_BUCKET_SHIFT;
    }
  return u;
}

bool
SfbQueueDisc::DropEarly (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this);
  double u =  m_uv->GetValue ();
  if (u <= GetMinProbability (item))
    {
      return true;
    }
  return false;
}

void
SfbQueueDisc::IncrementBinsQueueLength (Ptr<QueueDiscItem> item)
{

  uint32_t sfbhash = SfbHash (item);
  for (uint32_t i = 0; i < SFB_LEVELS; i++)
    {
      uint32_t hashed = sfbhash & SFB_BUCKET_MASK;
      m_bins.bins[i][hashed].packets++;
      if (m_bins.bins[i][hashed].packets > m_binSize)
        {
          IncrementBinPmark (i, hashed);
        }
      sfbhash >>= SFB_BUCKET_SHIFT;
    }
}

void
SfbQueueDisc::IncrementBinsPmarks (Ptr<QueueDiscItem> item)
{
  uint32_t sfbhash = SfbHash (item);
  for (uint32_t i = 0; i < SFB_LEVELS; i++)
    {
      uint32_t hashed = sfbhash & SFB_BUCKET_MASK;
      IncrementBinPmark (i, hashed);
      sfbhash >>= SFB_BUCKET_SHIFT;
    }
}

void
SfbQueueDisc::IncrementBinPmark (uint32_t i, uint32_t j)
{
  m_bins.bins[i][j].pmark += m_increment;
  if (m_bins.bins[i][j].pmark > 1.0)
    {
      m_bins.bins[i][j].pmark = 1.0;
    }
}

void
SfbQueueDisc::DecrementBinsQueueLength (Ptr<QueueDiscItem> item)
{
  uint32_t sfbhash = SfbHash (item);
  for (uint32_t i = 0; i < SFB_LEVELS; i++)
    {
      uint32_t hashed = sfbhash & SFB_BUCKET_MASK;
      m_bins.bins[i][hashed].packets--;
      if (m_bins.bins[i][hashed].packets < 0)
        {
          m_bins.bins[i][hashed].packets = 0;
        }
      sfbhash >>= SFB_BUCKET_SHIFT;
    }
}

void
SfbQueueDisc::DecrementBinPmark (uint32_t i, uint32_t j)
{
  m_bins.bins[i][j].pmark -= m_decrement;
  if (m_bins.bins[i][j].pmark < 0.0)
    {
      m_bins.bins[i][j].pmark = 0.0;
    }
}


Ptr<QueueDiscItem>
SfbQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ());

  NS_LOG_LOGIC ("Popped " << item);

  if (item)
    {
      DecrementBinsQueueLength (item);
    }

  return item;
}

Ptr<const QueueDiscItem>
SfbQueueDisc::DoPeek () const
{
  NS_LOG_FUNCTION (this);
  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<const QueueDiscItem> item = StaticCast<const QueueDiscItem> (GetInternalQueue (0)->Peek ());

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return item;
}

bool
SfbQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("SfbQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () == 0)
    {
      Ptr<SFBIpv4PacketFilter> ipv4Filter = CreateObject<SFBIpv4PacketFilter> ();
      AddPacketFilter (ipv4Filter);
      Ptr<SFBIpv6PacketFilter> ipv6Filter = CreateObject<SFBIpv6PacketFilter> ();
      AddPacketFilter (ipv6Filter);
    }

  if (GetNPacketFilters () != 2)
    {
      NS_LOG_ERROR ("SfbQueueDisc needs 2 filter");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // create a DropTail queue
      Ptr<InternalQueue> queue = CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> > ("Mode", EnumValue (m_mode));
      if (m_mode == QUEUE_DISC_MODE_PACKETS)
        {
          queue->SetMaxPackets (m_queueLimit);
        }
      else
        {
          queue->SetMaxBytes (m_queueLimit);
        }
      AddInternalQueue (queue);
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("SfbQueueDisc needs 1 internal queue");
      return false;
    }

  if ((GetInternalQueue (0)->GetMode () == QueueBase::QUEUE_MODE_PACKETS && m_mode == QUEUE_DISC_MODE_BYTES)
      || (GetInternalQueue (0)->GetMode () == QueueBase::QUEUE_MODE_BYTES && m_mode == QUEUE_DISC_MODE_PACKETS))
    {
      NS_LOG_ERROR ("The mode of the provided queue does not match the mode set on the SfbQueueDisc");
      return false;
    }

  if ((m_mode ==  QUEUE_DISC_MODE_PACKETS && GetInternalQueue (0)->GetMaxPackets () < m_queueLimit)
      || (m_mode ==  QUEUE_DISC_MODE_BYTES && GetInternalQueue (0)->GetMaxBytes () < m_queueLimit))
    {
      NS_LOG_ERROR ("The size of the internal queue is less than the queue disc limit");
      return false;
    }

  return true;
}

} //namespace ns3
