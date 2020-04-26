#include "chunk.h"

void ChunkProcessor::processEdges(Rect* r, std::pair<chunkProcessorFunctor, BiomeType*> f)
{
  Rect top { chunk->x1, chunk->y1, chunk->x2, chunk->y1 };
  Rect bottom { chunk->x1, chunk->y2, chunk->x2, chunk->y2 };
  Rect left { chunk->x1, chunk->y1, chunk->x1, chunk->y2 };
  Rect right { chunk->x2, chunk->y1, chunk->x2, chunk->y2 };
  std::vector<Rect> edges { top, bottom, left, right };
  for (auto it = edges.begin(); it != edges.end(); ++it)
  {
    std::thread t([this, f](int x1, int y1, int x2, int y2) {
      for (auto h = 0; h < zMax; h++)
        for (auto i = x1; i <= x2; i++)
          for (auto j = y1; j <= y2; j++)
            f.first(h, i, j, f.second);
    }, it->x1, it->y1, it->x2, it->y2);
    t.detach();
  }
}
void ChunkProcessor::multiProcess (Rect* r, std::array<std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*()>>>, 2> functors, int fuzz = 1)
{
  // Process every tile in every chunk
  for (auto it = smallchunks->begin(); it != smallchunks->end(); ++it)
  {
    // Process tiles in chunk thread
    for (auto f : functors[0])
    {
      BiomeType* b = f.second();
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Generating '%s' chunk", b->name.c_str());
      std::thread t([this, f, b, functors, fuzz](int x1, int y1, int x2, int y2) {
        for (auto h = 0; h < zMax; h += std::rand() % fuzz + 1)
          for (auto i = x1; i < x2; i += std::rand() % fuzz + 1)
            for (auto j = y1; j < y2; j += std::rand() % fuzz + 1)
              std::get<chunkProcessorFunctor>(f.first)(h, i, j, b);
      }, it->x1, it->y1, it->x2, it->y2);
      t.join();
    }
    // Post-process chunk thread
    for (auto f : functors[1])
    {
      BiomeType* b = f.second();
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Post-processing chunk (%d, %d)", it->x1, it->x2);
      std::thread t([this, &f, &b, it]() {
        std::get<chunkProcessorCallbackFunctor>(f.first)(&(*it), b);
      });
      t.join();
    }
  }
}
void ChunkProcessor::lazyProcess (Rect* r, std::vector<chunkFunctor> functors, int fuzz = 1)
{
  for (auto h = 0; h < zMax; h += std::rand() % fuzz + 1)
    for (auto i = r->x1; i < r->x2; i += std::rand() % fuzz + 1)
      for (auto j = r->y1; j < r->y2; j += std::rand() % fuzz + 1)
        for (auto f : functors) f(h, i, j);
}
void ChunkProcessor::process (Rect* r, std::vector<chunkFunctor> functors)
{
  for (auto h = 0; h < zMax; h++)
    for (auto i = r->x1; i != r->x2; i++)
      for (auto j = r->y1; j != r->y2; j++)
        for (auto f : functors) f(h, i, j);
}