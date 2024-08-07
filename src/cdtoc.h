#pragma once

#include <sega_cdc.h>

// -------------------------------------
// Constants
// -------------------------------------

/** @brief maximal number of tracks on disk
 */
#define CD_TRACK_COUNT (48)

// -------------------------------------
// Macros
// -------------------------------------

/** @brief Get track type flags
 *  @param control Track control field
 */
#define CDTrackGetTypeFlags(track) (((track)->Control))

/** @brief Check whether track is audio
 *  @param type Track type flag
 */
#define CDTrackIsAudio(track) ((CDTrackGetTypeFlags(track) & 0x04) == 0x00)

/** @brief Check whether track is 4 channel audio
 *  @param type Track type flag
 */
#define CDTrackIsAudio4Channel(track) ((CDTrackGetTypeFlags(track) & 0x0C) == 0x08)

/** @brief Check whether audio track has pre-emphasis
 *  @param type Track type flag
 */
#define CDTrackIsAudioWithPreEmphasis(track) ((CDTrackGetTypeFlags(track) & 0x05) == 0x01)

/** @brief Check whether track is data, recorded uninterrupted
 *  @param type Track type flag
 */
#define CDTrackIsData(track) ((CDTrackGetTypeFlags(track) & 0x0C) == 0x04)

/** @brief Check whether track is data, recorded incrementally
 *  @param type Track type flag
 */
#define CDTrackIsDataIncremental(track) ((CDTrackGetTypeFlags(track) & 0x0D) == 0x05)

/** @brief Check whether digital copy of this track is permitted
 *  @param type Track type flag
 */
#define CDTrackIsCopyPermitted(track) ((CDTrackGetTypeFlags(track) & 0x02) == 0x02)

/** @brief Check whether track is empty
 *  @param type Track type flag
 */
#define CDTrackIsEmpty(track) (CDTrackGetTypeFlags(track) == 0x0F)

// -------------------------------------
// Types
// -------------------------------------

/** @brief Track location data
 */
typedef struct
{
    unsigned int Control:4;
    unsigned int Number:4;
    unsigned int fad:24;
} CDTrackLocation;

/** @brief Track information data
 */
typedef struct
{
    unsigned char Control:4;
    unsigned char Address:4;
    unsigned char Number;
	union {
		short point;
		struct {
			char psec;
			char pframe;
		} pData;
		
	}pBody;
	
} CDTrackInformation;

/** @brief Session data
 */
typedef struct
{
    unsigned int Control:4;
    unsigned int Address:4;
    unsigned int fad:24;
} CDSession;

/** @brief Table of contents
 */
typedef struct
{
    CDTrackLocation Tracks[CD_TRACK_COUNT];
    CDTrackInformation FirstTrack;
    CDTrackInformation LastTrack;
    CDSession Session;
} CDTableOfContents;

