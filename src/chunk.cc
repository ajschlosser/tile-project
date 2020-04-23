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
      {
        for (auto i = x1; i <= x2; i++)
        {
          for (auto j = y1; j <= y2; j++)
          {
            f.first(h, i, j, f.second);
          }
        }
      }
    }, it->x1, it->y1, it->x2, it->y2);
    t.detach();
  }
}
void ChunkProcessor::multiProcess (Rect* r, std::array<std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*()>>>, 2> functors)
{
  for (auto it = smallchunks->begin(); it != smallchunks->end(); ++it)
  {
    for (auto f : functors[0])
    {
      BiomeType* b = f.second();
      std::thread t([this, f, b, functors](int x1, int y1, int x2, int y2) {
        for (auto h = 0; h < zMax; h++)
        {
          for (auto i = x1; i != x2; i++)
          {
            for (auto j = y1; j != y2; j++)
            {
              std::get<chunkProcessorFunctor>(f.first)(h, i, j, b);
            }
          }
        }
      }, it->x1, it->y1, it->x2, it->y2);
      t.detach();
    }
    // chunk postprocess
    for (auto f : functors[1])
    {

    }
  }
}
void ChunkProcessor::process (Rect* r, std::vector<chunkFunctor> functors)
{
  for (auto h = 0; h < zMax; h++)
  {
    for (auto i = r->x1; i != r->x2; i++)
    {
      for (auto j = r->y1; j != r->y2; j++)
      {
        for (auto f : functors) f(h, i, j);
      }
    }
  }
}