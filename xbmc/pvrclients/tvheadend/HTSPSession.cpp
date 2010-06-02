/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "HTSPSession.h"
#include "client.h"
#ifdef _MSC_VER
#include <winsock2.h>
#define SHUT_RDWR SD_BOTH
#define ETIMEDOUT WSAETIMEDOUT
#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

extern "C" {
#include "libhts/net.h"
#include "libhts/htsmsg.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

using namespace std;

cHTSPSession::cHTSPSession()
  : m_fd(INVALID_SOCKET)
  , m_seq(0)
  , m_challenge(NULL)
  , m_challenge_len(0)
  , m_protocol(0)
  , m_queue_size(1000)
{
}

cHTSPSession::~cHTSPSession()
{
  Close();
}

void cHTSPSession::Abort()
{
  shutdown(m_fd, SHUT_RDWR);
}

void cHTSPSession::Close()
{
  if(m_fd != INVALID_SOCKET)
  {
    closesocket(m_fd);
    m_fd = INVALID_SOCKET;
  }

  if(m_challenge)
  {
    free(m_challenge);
    m_challenge     = NULL;
    m_challenge_len = 0;
  }
}

bool cHTSPSession::Connect(const std::string& hostname, int port)
{
  char errbuf[1024];
  int  errlen = sizeof(errbuf);
  htsmsg_t *m;
  const char *method, *server, *version;
  const void * chall = NULL;
  size_t chall_len = 0;
  int32_t proto = 0;

  if(port == 0)
    port = 9982;

  m_fd = htsp_tcp_connect(hostname.c_str()
                        , port
                        , errbuf, errlen, 3000);
  if(m_fd == INVALID_SOCKET)
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::Open - failed to connect to server (%s)\n", errbuf);
    return false;
  }

  // send hello
  m = htsmsg_create_map();
  htsmsg_add_str(m, "method", "hello");
  htsmsg_add_str(m, "clientname", "XBMC Media Center");
  htsmsg_add_u32(m, "htspversion", 1);

  // read welcome
  if((m = ReadResult(m)) == NULL)
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::Open - failed to read greeting from server");
    return false;
  }
  method  = htsmsg_get_str(m, "method");
            htsmsg_get_s32(m, "htspversion", &proto);
  server  = htsmsg_get_str(m, "servername");
  version = htsmsg_get_str(m, "serverversion");
            htsmsg_get_bin(m, "challenge", &chall, &chall_len);

  XBMC->Log(LOG_DEBUG, "cHTSPSession::Open - connected to server: [%s], version: [%s], proto: %d"
                    , server ? server : "", version ? version : "", proto);

  m_server   = server;
  m_version  = version;
  m_protocol = proto;

  if(chall && chall_len)
  {
    m_challenge     = malloc(chall_len);
    m_challenge_len = chall_len;
    memcpy(m_challenge, chall, chall_len);
  }

  htsmsg_destroy(m);
  return true;
}

bool cHTSPSession::Auth(const std::string& username, const std::string& password)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"  , "authenticate");
  htsmsg_add_str(m, "username", username.c_str());

  if(password != "" && m_challenge)
  {
    struct HTSSHA1* shactx = (struct HTSSHA1*) malloc(hts_sha1_size);
    uint8_t d[20];
    hts_sha1_init(shactx);
    hts_sha1_update(shactx
                 , (const uint8_t *)password.c_str()
                 , password.length());
    hts_sha1_update(shactx
                 , (const uint8_t *)m_challenge
                 , m_challenge_len);
    hts_sha1_final(shactx, d);
    htsmsg_add_bin(m, "digest", d, 20);
    free(shactx);
  }

  return ReadSuccess(m, false, "get reply from authentication with server");
}

htsmsg_t* cHTSPSession::ReadMessage(int timeout)
{
  void*    buf;
  uint32_t l;
  int      x;

  if(m_queue.size())
  {
    htsmsg_t* m = m_queue.front();
    m_queue.pop_front();
    return m;
  }

  x = htsp_tcp_read_timeout(m_fd, &l, 4, timeout);
  if(x == ETIMEDOUT)
    return htsmsg_create_map();

  if(x)
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::ReadMessage - Failed to read packet size (%d)\n", x);
    return NULL;
  }

  l   = ntohl(l);
  if(l == 0)
    return htsmsg_create_map();

  buf = malloc(l);

  x = htsp_tcp_read(m_fd, buf, l);
  if(x)
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::ReadMessage - Failed to read packet (%d)\n", x);
    free(buf);
    return NULL;
  }

  return htsmsg_binary_deserialize(buf, l, buf); /* consumes 'buf' */
}

bool cHTSPSession::SendMessage(htsmsg_t* m)
{
  void*  buf;
  size_t len;

  if(htsmsg_binary_serialize(m, &buf, &len, -1) < 0)
  {
    htsmsg_destroy(m);
    return false;
  }
  htsmsg_destroy(m);

  if(send(m_fd, (char*)buf, len, 0) < 0)
  {
    free(buf);
    return false;
  }
  free(buf);
  return true;
}

htsmsg_t* cHTSPSession::ReadResult(htsmsg_t* m, bool sequence)
{
  if(sequence)
    htsmsg_add_u32(m, "seq", ++m_seq);

  if(!SendMessage(m))
    return NULL;

  std::deque<htsmsg_t*> queue;
  m_queue.swap(queue);

  while((m = ReadMessage()))
  {
    uint32_t seq;
    if(!sequence)
      break;
    if(!htsmsg_get_u32(m, "seq", &seq) && seq == m_seq)
      break;

    queue.push_back(m);
    if(queue.size() >= m_queue_size)
    {
      XBMC->Log(LOG_ERROR, "CDVDInputStreamHTSP::ReadResult - maximum queue size (%u) reached", m_queue_size);
      m_queue.swap(queue);
      return NULL;
    }
  }

  m_queue.swap(queue);

  const char* error;
  if(m && (error = htsmsg_get_str(m, "error")))
  {
    XBMC->Log(LOG_ERROR, "CDVDInputStreamHTSP::ReadResult - error (%s)", error);
    htsmsg_destroy(m);
    return NULL;
  }
  uint32_t noaccess;
  if(m && !htsmsg_get_u32(m, "noaccess", &noaccess) && noaccess)
  {
    XBMC->Log(LOG_ERROR, "CDVDInputStreamHTSP::ReadResult - access denied (%d)", noaccess);
    htsmsg_destroy(m);
    return NULL;
  }

  return m;
}

bool cHTSPSession::ReadSuccess(htsmsg_t* m, bool sequence, std::string action)
{
  if((m = ReadResult(m, sequence)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "CDVDInputStreamHTSP::ReadSuccess - failed to %s", action.c_str());
    return false;
  }
  htsmsg_destroy(m);
  return true;
}

bool cHTSPSession::SendSubscribe(int subscription, int channel)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"        , "subscribe");
  htsmsg_add_s32(m, "channelId"     , channel);
  htsmsg_add_s32(m, "subscriptionId", subscription);
  return ReadSuccess(m, true, "subscribe to channel");
}

bool cHTSPSession::SendUnsubscribe(int subscription)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"        , "unsubscribe");
  htsmsg_add_s32(m, "subscriptionId", subscription);
  return ReadSuccess(m, true, "unsubscribe from channel");
}

bool cHTSPSession::SendEnableAsync()
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method", "enableAsyncMetadata");
  return ReadSuccess(m, true, "enableAsyncMetadata failed");
}

bool cHTSPSession::GetEvent(SEvent& event, uint32_t id)
{
  if(id == 0)
  {
    event.Clear();
    return false;
  }

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getEvent");
  htsmsg_add_u32(msg, "eventId", id);
  if((msg = ReadResult(msg, true)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPSession::GetEvent - failed to get event %d", id);
    return false;
  }
  return ParseEvent(msg, id, event);
}

bool cHTSPSession::ParseEvent(htsmsg_t* msg, uint32_t id, SEvent &event)
{
  uint32_t start, stop, next, chan_id, content;
  const char *title, *desc;
  if(         htsmsg_get_u32(msg, "start", &start)
  ||          htsmsg_get_u32(msg, "stop" , &stop)
  || (title = htsmsg_get_str(msg, "title")) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPSession::ParseEvent - malformed event");
    htsmsg_print(msg);
    htsmsg_destroy(msg);
    return false;
  }
  event.Clear();
  event.id    = id;
  event.start = start;
  event.stop  = stop;
  event.title = title;

  if((desc = htsmsg_get_str(msg, "description")))
    event.descs = desc;
  if(htsmsg_get_u32(msg, "nextEventId", &next))
    event.next = 0;
  else
    event.next = next;
  if(htsmsg_get_u32(msg, "channelId", &chan_id))
    event.chan_id = -1;
  else
    event.chan_id = chan_id;
  if(htsmsg_get_u32(msg, "contentType", &content))
    event.content = -1;
  else
    event.content = content;

  XBMC->Log(LOG_DEBUG, "cHTSPSession::ParseEvent - id:%u, chan_id:%u, title:'%s', desc:'%s', start:%u, stop:%u, next:%u"
                    , event.id
                    , event.chan_id
                    , event.title.c_str()
                    , event.descs.c_str()
                    , event.start
                    , event.stop
                    , event.next);

  return true;
}

void cHTSPSession::ParseChannelUpdate(htsmsg_t* msg, SChannels &channels)
{
  uint32_t id, event = 0, num = 0;
  const char *name, *icon;
  if(htsmsg_get_u32(msg, "channelId", &id))
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::ParseChannelUpdate - malformed message received");
    htsmsg_print(msg);
    return;
  }

  SChannel &channel = channels[id];
  channel.id = id;

  if(htsmsg_get_u32(msg, "eventId", &event) == 0)
    channel.event = event;

  if((name = htsmsg_get_str(msg, "channelName")))
    channel.name = name;

  if((icon = htsmsg_get_str(msg, "channelIcon")))
    channel.icon = icon;

  if(htsmsg_get_u32(msg, "channelNumber", &num) == 0)
  {
    if(num == 0)
      channel.num = id + 1000;
    else
      channel.num = num;
  }
  else
    channel.num = id; // fallback older servers

  htsmsg_t *tags;

  if((tags = htsmsg_get_list(msg, "tags")))
  {
    channel.tags.clear();

    htsmsg_field_t *f;
    HTSMSG_FOREACH(f, tags)
    {
      if(f->hmf_type != HMF_S64)
        continue;
      channel.tags.push_back((int)f->hmf_s64);
    }
  }


  XBMC->Log(LOG_DEBUG, "cHTSPSession::ParseChannelUpdate - id:%u, name:'%s', icon:'%s', event:%u"
                    , id, name ? name : "(null)", icon ? icon : "(null)", event);
}

void cHTSPSession::ParseChannelRemove(htsmsg_t* msg, SChannels &channels)
{
  uint32_t id;
  if(htsmsg_get_u32(msg, "channelId", &id))
  {
    XBMC->Log(LOG_ERROR, "CDVDInputStreamHTSP::ParseChannelRemove - malformed message received");
    htsmsg_print(msg);
    return;
  }
  XBMC->Log(LOG_DEBUG, "cHTSPSession::ParseChannelRemove - id:%u", id);

  channels.erase(id);
}

void cHTSPSession::ParseTagUpdate(htsmsg_t* msg, STags &tags)
{
  uint32_t id;
  const char *name, *icon;
  if(htsmsg_get_u32(msg, "tagId", &id))
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::ParseTagUpdate - malformed message received");
    htsmsg_print(msg);
    return;
  }
  STag &tag = tags[id];
  tag.id = id;

  if((icon = htsmsg_get_str(msg, "tagIcon")))
    tag.icon  = icon;

  if((name = htsmsg_get_str(msg, "tagName")))
    tag.name  = name;

  htsmsg_t *channels;

  if((channels = htsmsg_get_list(msg, "members")))
  {
    tag.channels.clear();

    htsmsg_field_t *f;
    HTSMSG_FOREACH(f, channels)
    {
      if(f->hmf_type != HMF_S64)
        continue;
      tag.channels.push_back((int)f->hmf_s64);
    }
  }

  XBMC->Log(LOG_DEBUG, "cHTSPSession::ParseTagUpdate - id:%u, name:'%s', icon:'%s'"
                    , id, name ? name : "(null)", icon ? icon : "(null)");

}

void cHTSPSession::ParseTagRemove(htsmsg_t* msg, STags &tags)
{
  uint32_t id;
  if(htsmsg_get_u32(msg, "tagId", &id))
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::ParseTagRemove - malformed message received");
    htsmsg_print(msg);
    return;
  }
  XBMC->Log(LOG_DEBUG, "cHTSPSession::ParseTagRemove - id:%u", id);

  tags.erase(id);
}

bool cHTSPSession::ParseQueueStatus (htsmsg_t* msg, SQueueStatus &queue, SQuality &quality)
{
  if(htsmsg_get_u32(msg, "packets", &queue.packets)
  || htsmsg_get_u32(msg, "bytes",   &queue.bytes)
  || htsmsg_get_u32(msg, "Bdrops",  &queue.bdrops)
  || htsmsg_get_u32(msg, "Pdrops",  &queue.pdrops)
  || htsmsg_get_u32(msg, "Idrops",  &queue.idrops))
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::ParseQueueStatus - malformed message received");
    htsmsg_print(msg);
    return false;
  }

  /* delay isn't always transmitted */
  if(htsmsg_get_u32(msg, "delay", &queue.delay))
    queue.delay = 0;

  if(htsmsg_get_u32(msg, "feSNR", &quality.fe_snr))
    quality.fe_snr = -2;

  if(htsmsg_get_u32(msg, "feSignal", &quality.fe_signal))
    quality.fe_signal = -2;

  if(htsmsg_get_u32(msg, "feBER", &quality.fe_ber))
    quality.fe_ber = -2;

  if(htsmsg_get_u32(msg, "feUNC", &quality.fe_unc))
    quality.fe_unc = -2;

  const char* name;
  if((name = htsmsg_get_str(msg, "feName")) == NULL)
    quality.fe_name = "";
  else
    quality.fe_name = name;

  const char* status;
  if((status = htsmsg_get_str(msg, "feStatus")) == NULL)
    quality.fe_status = "";
  else
    quality.fe_status = status;

  return true;
}

void cHTSPSession::ParseDVREntryAdd(htsmsg_t* msg, SRecordings &recordings)
{
  SRecording recording;

  if(htsmsg_get_u32(msg, "id",      &recording.id)
  || htsmsg_get_u32(msg, "channel", &recording.channel)
  || htsmsg_get_u32(msg, "start",   &recording.start)
  || htsmsg_get_u32(msg, "stop",    &recording.stop))
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::ParseDVREntryAdd - malformed message received");
    htsmsg_print(msg);
    return;
  }

  const char* str;
  if((str = htsmsg_get_str(msg, "title")) == NULL)
    recording.title = "";
  else
    recording.title = str;

  if((str = htsmsg_get_str(msg, "description")) == NULL)
    recording.description = "";
  else
    recording.description = str;

  XBMC->Log(LOG_DEBUG, "cHTSPSession::ParseDVREntryAdd - id:%u title '%s'\ndescription '%s'", recording.id, recording.title.c_str(), recording.description.c_str());

  recordings[recording.id] = recording;
}

void cHTSPSession::ParseDVREntryDelete(htsmsg_t* msg, SRecordings &recordings)
{
  uint32_t id;

  if(htsmsg_get_u32(msg, "id", &id))
  {
    XBMC->Log(LOG_ERROR, "cHTSPSession::ParseDVREntryDelete - malformed message received");
    htsmsg_print(msg);
    return;
  }

  XBMC->Log(LOG_DEBUG, "cHTSPSession::ParseDVREntryDelete - Recording %i was deleted", id);

  recordings.erase(id);
}