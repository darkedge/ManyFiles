#include "pch.h"
#include "mj_common.h"

size_t mj::Kibibytes(size_t kib)
{
  return kib * 1024;
}

size_t mj::Mebibytes(size_t mib)
{
  return mib * Kibibytes(1024);
}

size_t mj::Gibibytes(size_t gib)
{
  return gib * Mebibytes(1024);
}

size_t mj::Tebibytes(size_t tib)
{
  return tib * Gibibytes(1024);
}
