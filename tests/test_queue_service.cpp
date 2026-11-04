#include "core/services/PlayerService.hpp"
#include "core/services/QueueService.hpp"
#include "mocks/MockAudioEnginePort.hpp"
#include <gtest/gtest.h>

namespace {

core::Track makeTrack(const std::string &id, const std::string &title,
                      const std::string &artist, const std::string &album,
                      uint32_t trackNum = 1, uint64_t dur = 200000) {
  core::Track t;
  t.id = id;
  t.filePath = "/music/" + id + ".flac";
  t.title = title;
  t.artist = artist;
  t.album = album;
  t.albumArtist = artist;
  t.trackNumber = trackNum;
  t.durationMs = dur;
  return t;
}

} // namespace

TEST(QueueServiceTest, PlayNowInitializesQueueAndStream) {
  core::QueueService queue(42);

  auto t1 = makeTrack("1", "Song 1", "Artist A", "Album A", 1);
  auto t2 = makeTrack("2", "Song 2", "Artist A", "Album A", 2);
  auto t3 = makeTrack("3", "Song 3", "Artist A", "Album A", 3);

  std::vector<core::Track> albumTracks = {t1, t2, t3};
  queue.playNow(t1, albumTracks, 0);

  ASSERT_TRUE(queue.getCurrentTrack().has_value());
  EXPECT_EQ(queue.getCurrentTrack()->id, "1");
  EXPECT_EQ(queue.getRemainingCount(), 2u);

  auto next1 = queue.next();
  ASSERT_TRUE(next1.has_value());
  EXPECT_EQ(next1->id, "2");

  auto next2 = queue.next();
  ASSERT_TRUE(next2.has_value());
  EXPECT_EQ(next2->id, "3");

  // Queue ends with repeat off
  EXPECT_FALSE(queue.next().has_value());
}

TEST(QueueServiceTest, UpNextPriorityTakesPrecedenceOverMainStream) {
  core::QueueService queue(42);

  auto t1 = makeTrack("1", "Stream 1", "Artist A", "Album A", 1);
  auto t2 = makeTrack("2", "Stream 2", "Artist A", "Album A", 2);
  auto t3 = makeTrack("3", "Stream 3", "Artist A", "Album A", 3);

  queue.playNow(t1, {t1, t2, t3}, 0);

  auto priority1 = makeTrack("p1", "Priority Next", "Guest", "Single");
  auto priority2 = makeTrack("p2", "Priority Last", "Guest", "Single");

  queue.playNext(priority1);  // Front of Up Next
  queue.queueLast(priority2); // Back of Up Next

  EXPECT_EQ(queue.getRemainingCount(), 4u);

  // Next track must be priority1
  auto n1 = queue.next();
  ASSERT_TRUE(n1.has_value());
  EXPECT_EQ(n1->id, "p1");

  // Next track must be priority2
  auto n2 = queue.next();
  ASSERT_TRUE(n2.has_value());
  EXPECT_EQ(n2->id, "p2");

  // After Up Next is exhausted, continue along Main Stream
  auto n3 = queue.next();
  ASSERT_TRUE(n3.has_value());
  EXPECT_EQ(n3->id, "2");

  auto n4 = queue.next();
  ASSERT_TRUE(n4.has_value());
  EXPECT_EQ(n4->id, "3");
}

TEST(QueueServiceTest, ReversibleHistoryStackNavigatesBackwards) {
  core::QueueService queue(42);

  auto t1 = makeTrack("1", "Song 1", "Artist", "Album");
  auto t2 = makeTrack("2", "Song 2", "Artist", "Album");
  auto t3 = makeTrack("3", "Song 3", "Artist", "Album");

  queue.playNow(t1, {t1, t2, t3}, 0);
  EXPECT_EQ(queue.next()->id, "2"); // plays t2
  EXPECT_EQ(queue.next()->id, "3"); // plays t3

  EXPECT_EQ(queue.getCurrentTrack()->id, "3");

  // Go backwards using reversible history stack
  auto prev1 = queue.previous();
  ASSERT_TRUE(prev1.has_value());
  EXPECT_EQ(prev1->id, "2");

  auto prev2 = queue.previous();
  ASSERT_TRUE(prev2.has_value());
  EXPECT_EQ(prev2->id, "1");
}

TEST(QueueServiceTest, RepeatModes) {
  core::QueueService queue(42);

  auto t1 = makeTrack("1", "Song 1", "Artist", "Album");
  auto t2 = makeTrack("2", "Song 2", "Artist", "Album");

  queue.playNow(t1, {t1, t2}, 0);

  // Repeat One
  queue.setRepeatMode(core::RepeatMode::One);
  auto repeatSame = queue.next();
  ASSERT_TRUE(repeatSame.has_value());
  EXPECT_EQ(repeatSame->id, "1");

  // Repeat All
  queue.setRepeatMode(core::RepeatMode::All);
  EXPECT_EQ(queue.next()->id, "2"); // plays t2
  auto loopFirst = queue.next();    // loops back to t1
  ASSERT_TRUE(loopFirst.has_value());
  EXPECT_EQ(loopFirst->id, "1");

  // Repeat Off
  queue.setRepeatMode(core::RepeatMode::Off);
  EXPECT_EQ(queue.next()->id, "2"); // plays t2
  auto shouldEnd = queue.next();    // ends
  EXPECT_FALSE(shouldEnd.has_value());
}

TEST(QueueServiceTest, AlbumShufflePreservesTrackOrderWithinAlbums) {
  core::QueueService queue(12345);

  // 2 Albums, 2 tracks each
  auto a1_t1 = makeTrack("a1_1", "Intro", "Artist A", "Album 1", 1);
  auto a1_t2 = makeTrack("a1_2", "Outro", "Artist A", "Album 1", 2);
  auto a2_t1 = makeTrack("a2_1", "Prelude", "Artist B", "Album 2", 1);
  auto a2_t2 = makeTrack("a2_2", "Finale", "Artist B", "Album 2", 2);

  std::vector<core::Track> library = {a1_t1, a1_t2, a2_t1, a2_t2};
  queue.playNow(a1_t1, library, 0);

  // Enable Album Shuffle
  queue.setShuffleMode(core::ShuffleMode::Albums);

  auto upcoming = queue.getUpcomingQueue();
  ASSERT_EQ(upcoming.size(), 3u);

  // Within each album, trackNumber must always be ascending
  for (size_t i = 0; i + 1 < upcoming.size(); ++i) {
    if (upcoming[i].album == upcoming[i + 1].album) {
      EXPECT_LE(upcoming[i].trackNumber, upcoming[i + 1].trackNumber);
    }
  }

  // Disabling shuffle restores original order non-destructively
  queue.setShuffleMode(core::ShuffleMode::Off);
  auto restoredUpcoming = queue.getUpcomingQueue();
  EXPECT_EQ(restoredUpcoming[0].id, "a1_2");
  EXPECT_EQ(restoredUpcoming[1].id, "a2_1");
  EXPECT_EQ(restoredUpcoming[2].id, "a2_2");
}

TEST(QueueServiceTest, QueueMutationsAndReordering) {
  core::QueueService queue(42);

  auto t1 = makeTrack("1", "Song 1", "Artist", "Album");
  auto t2 = makeTrack("2", "Song 2", "Artist", "Album");
  auto t3 = makeTrack("3", "Song 3", "Artist", "Album");
  auto t4 = makeTrack("4", "Song 4", "Artist", "Album");

  queue.playNow(t1, {t1, t2, t3, t4}, 0);
  EXPECT_EQ(queue.getRemainingCount(), 3u);

  // Remove item at index 1 ("Song 3")
  EXPECT_TRUE(queue.removeFromQueue(1));
  EXPECT_EQ(queue.getRemainingCount(), 2u);

  auto upcoming = queue.getUpcomingQueue();
  EXPECT_EQ(upcoming[0].id, "2");
  EXPECT_EQ(upcoming[1].id, "4");

  // Move item at index 1 to index 0 (reorder 4 before 2)
  EXPECT_TRUE(queue.moveQueueItem(1, 0));
  upcoming = queue.getUpcomingQueue();
  EXPECT_EQ(upcoming[0].id, "4");
  EXPECT_EQ(upcoming[1].id, "2");

  // Clear queue
  queue.clearQueue();
  EXPECT_EQ(queue.getRemainingCount(), 0u);
}

TEST(QueueServiceTest, PlayerServiceIntegrationWithMockAudio) {
  auto mockAudio = std::make_shared<tests::MockAudioEnginePort>();
  core::PlayerService player(mockAudio);

  auto t1 = makeTrack("1", "Track 1", "Artist", "Album");
  auto t2 = makeTrack("2", "Track 2", "Artist", "Album");

  player.playQueue({t1, t2}, 0);
  EXPECT_TRUE(player.isPlaying());
  EXPECT_EQ(mockAudio->m_loadedFilePath, "/music/1.flac");

  player.next();
  EXPECT_EQ(mockAudio->m_loadedFilePath, "/music/2.flac");

  player.previous();
  EXPECT_EQ(mockAudio->m_loadedFilePath, "/music/1.flac");
}

TEST(QueueServiceTest, RestoreStateRebuildsQueueAndHistory) {
  core::QueueService queue(42);

  auto curr = makeTrack("curr", "Current Song", "Artist", "Album");
  auto u1 = makeTrack("u1", "Up Next 1", "Artist", "Album");
  auto u2 = makeTrack("u2", "Up Next 2", "Artist", "Album");
  auto m1 = makeTrack("m1", "Main 1", "Artist", "Album");
  auto m2 = makeTrack("m2", "Main 2", "Artist", "Album");
  auto m3 = makeTrack("m3", "Main 3", "Artist", "Album");
  auto h1 = makeTrack("h1", "History 1", "Artist", "Album");
  auto h2 = makeTrack("h2", "History 2", "Artist", "Album");

  std::vector<core::Track> upNext = {u1, u2};
  std::vector<core::Track> mainStream = {m1, m2, m3};
  std::vector<core::Track> history = {h1, h2};

  queue.restoreState(curr, upNext, mainStream, 1, history);

  ASSERT_TRUE(queue.getCurrentTrack().has_value());
  EXPECT_EQ(queue.getCurrentTrack()->id, "curr");
  EXPECT_EQ(queue.getUpNext().size(), 2u);
  EXPECT_EQ(queue.getHistory().size(), 2u);
  EXPECT_EQ(queue.getHistory()[0].id, "h1");
  EXPECT_EQ(queue.getHistory()[1].id, "h2");

  // Upcoming queue should have upNext tracks followed by remaining mainStream
  // from index 2
  auto upcoming = queue.getUpcomingQueue();
  ASSERT_EQ(upcoming.size(), 3u);
  EXPECT_EQ(upcoming[0].id, "u1");
  EXPECT_EQ(upcoming[1].id, "u2");
  EXPECT_EQ(upcoming[2].id, "m3");

  // Navigate backwards in reversible history
  auto prev1 = queue.previous();
  ASSERT_TRUE(prev1.has_value());
  EXPECT_EQ(prev1->id, "h2");

  // Next track returns to curr
  auto n1 = queue.next();
  ASSERT_TRUE(n1.has_value());
  EXPECT_EQ(n1->id, "curr");

  // Next track consumes upNext u1
  auto n2 = queue.next();
  ASSERT_TRUE(n2.has_value());
  EXPECT_EQ(n2->id, "u1");
}

TEST(QueueServiceTest, PlayerServiceRestoreSessionPausedAndPlay) {
  auto mockAudio = std::make_shared<tests::MockAudioEnginePort>();
  core::PlayerService player(mockAudio);

  auto t1 = makeTrack("1", "Song 1", "Artist", "Album");
  auto t2 = makeTrack("2", "Song 2", "Artist", "Album");
  auto h1 = makeTrack("h1", "Hist 1", "Artist", "Album");

  // Restore paused
  player.restoreSession(t1, 45000, {t2}, {t1, t2}, 0, {h1}, false);

  EXPECT_FALSE(player.isPlaying());
  EXPECT_EQ(mockAudio->m_loadedFilePath, "/music/1.flac");
  EXPECT_EQ(mockAudio->m_positionMs, 45000u);
  ASSERT_TRUE(player.getCurrentTrack().has_value());
  EXPECT_EQ(player.getCurrentTrack()->id, "1");

  // Restore with autoResume = true
  player.restoreSession(t2, 12000, {}, {t1, t2}, 1, {h1, t1}, true);

  EXPECT_TRUE(player.isPlaying());
  EXPECT_EQ(mockAudio->m_loadedFilePath, "/music/2.flac");
  EXPECT_EQ(mockAudio->m_positionMs, 12000u);
  ASSERT_TRUE(player.getCurrentTrack().has_value());
  EXPECT_EQ(player.getCurrentTrack()->id, "2");
}
