#include "chunk.h"

// TODO: Integrate this into the map namespace

using namespace map::chunk;

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

void ChunkProcessor::multiProcess (Rect* r, multiprocessFunctorArray functors, int fuzz = 1)
{

  // Process every tile in every chunk
  for (auto it = smallchunks->begin(); it != smallchunks->end(); ++it)
  {
    for (auto f : functors[0])
    {
      BiomeType* b = f.second(this, 0, it->getMid());
      auto fn = std::get<chunkProcessorFunctor>(f.first);
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Generating '%s' chunk", b->name.c_str());
      int n = std::rand() % 10;
      std::thread t([this, n, fn, b, fuzz](int x1, int y1, int x2, int y2) {
        for (auto h = 0; h < zMax; h += std::rand() % fuzz + 1)
          for (auto i = n > 5 ? x1 : x2; [i,x2,x1,n](){if(n>5)return i<=x2;else return i>=x1;}() ; [&i,n, fuzz](){if(n>5)i+=std::rand()%fuzz+1;else i-=(std::rand()%fuzz+1);}())
            for (auto j = n > 5 ? y1 : y2; [j,y2,y1,n](){if(n>5)return j<=y2;else return j>=y1;}() ; [&j,n, fuzz](){if(n>5)j+=std::rand()%fuzz+1;else j-=(std::rand()%fuzz+1);}())
              fn(h, i, j, b);
      }, it->x1, it->y1, it->x2, it->y2);
      t.join();
    }
    // TODO: This may be unnecessary
    // TODO: Re-think map processing flows
    // Post-process chunk thread
    for (auto f : functors[1])
    {
      BiomeType* b = f.second(this, 0, it->getMid());
      auto fn = std::get<chunkProcessorCallbackFunctor>(f.first);
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Post-processing chunk (%d, %d)", it->x1, it->x2);
      std::thread t([this, fn, &b, it]() {
        fn(&(*it), b);
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

map::chunk::ChunkReport map::chunk::getRangeReport(std::function<void(int, int, map::chunk::ChunkReport*)> f, Rect* r, int fuzz = 1, int base = 1)
{
  ChunkReport report;
  for (auto i = r->x1; i < r->x2; i += base + std::rand() % fuzz)
    for (auto j = r->y1; j < r->y2; j += base + std::rand() % fuzz)
      f(i, j, &report);
  return report;
}