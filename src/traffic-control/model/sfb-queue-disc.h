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

#ifndef SFB_QUEUE_DISC_H
#define SFB_QUEUE_DISC_H

#define SFB_BUCKET_SHIFT 4
#define SFB_BINS (1 << SFB_BUCKET_SHIFT) /* N bins per Level */
#define SFB_BUCKET_MASK (SFB_BINS - 1)
#define SFB_LEVELS (32 / SFB_BUCKET_SHIFT) /* L */

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/timer.h"
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class UniformRandomVariable;

class SfbQueueDisc : public QueueDisc
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief SfbQueueDisc Constructor
   */
  SfbQueueDisc ();

  /**
   * \brief SfbQueueDisc Destructor
   */
  virtual ~SfbQueueDisc ();

  /**
   * \brief Bin
   */
  typedef struct
  {
    double pmark;
    uint32_t packets;
  } Bin_t;

  typedef struct
  {
    uint32_t   perturbation;     /* jhash perturbation */
    Bin_t      bins[SFB_LEVELS][SFB_BINS];
  } SfbBins_t;

  /**
   * \brief Enumeration of the modes supported in the class.
   *
   */
  enum QueueDiscMode
  {
    QUEUE_DISC_MODE_PACKETS,     /**< Use number of packets for maximum queue disc size */
    QUEUE_DISC_MODE_BYTES,       /**< Use number of bytes for maximum queue disc size */
  };

// Reasons for dropping packets
  static constexpr const char* TARGET_EXCED_DROP = "Target exceeded drop";  //!< above target
  static constexpr const char* OVERLIMIT_DROP = "Overlimit drop";  //!< Overlimit dropped packet
  static constexpr const char* RATELIMIT_DROP = "Ratelimit drop";  //!< Overlimit dropped packet
  static constexpr const char* BUCKETLIMIT_DROP = "Bucketlimit drop";  //!< Overlimit dropped packet

  /**
   * \brief Set the operating mode of this queue disc.
   *
   * \param mode The operating mode of this queue disc.
   */
  void SetMode (QueueDiscMode mode);

  /**
   * \brief Get the operating mode of this queue disc.
   *
   * \returns The operating mode of this queue disc.
   */
  QueueDiscMode GetMode (void);

  /**
   * \brief Get the current value of the queue in bytes or packets.
   *
   * \returns The queue size in bytes or packets.
   */
  uint32_t GetQueueSize (void);

  /**
   * \brief Set the limit of the queue in bytes or packets.
   *
   * \param lim The limit in bytes or packets.
   */
  void SetQueueLimit (uint32_t lim);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

protected:
  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

  /**
   * \brief Initialize the queue parameters.
   */
  virtual void InitializeParams (void);
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void) const;
  virtual bool CheckConfig (void);


private:
  void InitializeBins (void);
  double GetMinProbability (Ptr<QueueDiscItem> item);
  void IncrementBinsQueueLength (Ptr<QueueDiscItem> item);
  void IncrementBinsPmarks (Ptr<QueueDiscItem> item);
  void IncrementBinPmark (uint32_t i, uint32_t j);
  void DecrementBinsQueueLength (Ptr<QueueDiscItem> item);
  void DecrementBinPmark (uint32_t i, uint32_t j);
  uint32_t SfbHash (Ptr<QueueDiscItem> item);
  bool RateLimit (Ptr<QueueDiscItem> item);

  /**
   * \brief Check if a packet needs to be dropped due to probability drop
   * \returns false for no drop, true for drop
   */
  virtual bool DropEarly (Ptr<QueueDiscItem> item);


  Ptr<UniformRandomVariable> m_uv;              //!< Rng stream
  Ptr<UniformRandomVariable> m_pertbUV;         //!< Rng stream

  // ** Variables supplied by user
  QueueDiscMode m_mode;                      //!< Mode (bytes or packets)
  uint32_t m_queueLimit;                        //!< Queue limit in bytes / packets
  uint32_t m_meanPktSize;                       //!< Average Packet Size
  double m_increment;                           //!< increment value for marking probability
  double m_decrement;                           //!< decrement value for marking probability
  double m_freezeTime;
  Time   m_rehashInterval;                              //!<Time interval after which hash willbe changed
  uint32_t m_penaltyRate;                         //!<Panelty rate in packets per second
  uint32_t m_penaltyBurst;                        //!<Panelty burst in packets
  uint32_t m_tokenAvail; // Added now
  Time     m_tokenTime; // added now
  Time     m_interval;
  double   m_targetFraction;

  // ** Variables maintained by SFB
  uint32_t m_binSize;
  Time   m_lastSet;                             //!< Last time rehash was changed
  SfbBins_t m_bins;
  uint32_t m_bucketLimit;                          //!Bucket limit
  uint32_t m_target;
};

} // namespace ns3

#endif // BLUE_QUEUE_DISC_H
