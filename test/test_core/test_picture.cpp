// Picture: choosing pre-rendered artwork for the surface resolution.

#include "render/Picture.h"
#include "test_support.h"

using namespace support;

namespace {

const uint16_t kSmall[] = {0xFFFF};
const uint16_t kBig[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

Picture twoFaces() {
  const ImageView faces[] = {{1, 1, kSmall, nullptr, 1.0f}, {2, 2, kBig, nullptr, 2.0f}};
  return Picture(faces, 2);
}

int drawnPixels(const Picture& picture, int side) {
  SoftCanvas canvas(side, side);
  ScaledSurface surface(canvas);
  picture.draw(surface, 0, 0);
  return countNonBlack(canvas.image());
}

}  // namespace

TEST_CASE("picture: a single image is drawn at the design resolution") {
  CHECK(drawnPixels(Picture(ImageView{1, 1, kSmall}), 240) == 1);
}

TEST_CASE("picture: chooses the smallest face at least as large as the surface") {
  const Picture picture = twoFaces();
  CHECK(drawnPixels(picture, 240) == 1);  // 1x face, as it is
  CHECK(drawnPixels(picture, 480) == 4);  // 2x face, as it is
  CHECK(drawnPixels(picture, 360) == 4);  // one design pixel rounds to 2x2 at 1.5x
}

TEST_CASE("picture: beyond the largest face, that face is stretched") {
  CHECK(drawnPixels(twoFaces(), 720) == 9);  // one design pixel at 3x
}

TEST_CASE("picture: faces beyond the limit are ignored") {
  ImageView faces[Picture::kMaxFaces + 2];
  for (ImageView& face : faces) face = ImageView{1, 1, kSmall};
  const Picture picture(faces, Picture::kMaxFaces + 2);
  CHECK(drawnPixels(picture, 240) == 1);
}
