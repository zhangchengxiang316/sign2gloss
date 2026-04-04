/*
 * @Author: Jingwen Bai
 * @Date: 2024-07-04 11:07:00
 * @Description:
 * @Filename: osd-device.hpp
 */
#ifndef SST_OSD_DEVICE_HPP_
#define SST_OSD_DEVICE_HPP_

#include <vector>
#include <string>
#include <array>
#include <cstdint>

#include "osd_lib_api.h"
#include "common.hpp"

#define BUFFER_TYPE_DMABUF  0x1
#define OSD_LAYER_SIZE 5

namespace sst {
namespace device {
namespace osd {

// Draw primitive type: keep backward compatibility with old "box" mode,
// but add "vertex" mode to support true thin slanted lines (convex quadrangles).
enum class OsdShapeType {
    BOX = 0,     // Use [xmin, ymin, xmax, ymax] and border to generate inner/outer rectangles
    VERTEX = 1   // Use explicit vertex_out / vertex_in (true convex quadrangle)
};

typedef struct {
    // Mode switch
    OsdShapeType shape_type = OsdShapeType::BOX;

    // BOX mode payload: [xmin, ymin, xmax, ymax]
    std::array<float, 4> box = {0, 0, 0, 0};
    int border = 0;

    // VERTEX mode payload: explicit vertices in layer coordinate space
    fdevice::VERTEXS_S vertex_out = {};
    fdevice::VERTEXS_S vertex_in  = {};

    // Style
    int layer_id = 0;
    fdevice::QUADRANGLETYPE type = fdevice::TYPE_SOLID;
    fdevice::ALPHATYPE alpha = fdevice::TYPE_ALPHA75;
    int color = 0;
} OsdQuadRangle;

class OsdDevice {
public:
    OsdDevice();
    ~OsdDevice();

    void Initialize(int width, int height);
    void Release();

    // Auto layer allocation mode (library chooses a suitable TYPE_GRAPHIC layer)
    void Draw(std::vector<OsdQuadRangle>& quad_rangle);

    // Manual layer mode: draw on a specific layer
    void Draw(std::vector<OsdQuadRangle>& quad_rangle, int layer_id);

    // Legacy helper: draw axis-aligned rectangles from boxes
    void Draw(std::vector<std::array<float, 4>>& boxes,
              int border,
              int layer_id,
              fdevice::QUADRANGLETYPE type,
              fdevice::ALPHATYPE alpha,
              int color);

    // Explicit clear helpers
    void ClearAll();
    void ClearLayer(int layer_id);

private:
    int LoadLutFile(const char* filename);

    // BOX-mode helper: generate inner/outer vertices from [xmin,ymin,xmax,ymax] + border
    void GenQrangleBox(std::array<float, 4>& det, int border,
                       fdevice::VERTEXS_S& out_v,
                       fdevice::VERTEXS_S& in_v);

    // Clamp helpers
    inline int ClampX(int x) const { return std::min(m_width, std::max(0, x)); }
    inline int ClampY(int y) const { return std::min(m_height, std::max(0, y)); }

private:
    handle_t m_osd_handle = INVALID_HANDLE;
    std::string m_osd_lut_path = "/app_demo/app_assets/colorLUT.sscl";

    uint8_t* m_pcolor_lut = nullptr;
    int m_file_size = 0;

    int m_height = 0, m_width = 0;

    fdevice::DMA_BUFFER_ATTR_S m_layer_dma[OSD_LAYER_SIZE];
};

} // namespace osd
} // namespace device
} // namespace sst

#endif // SST_OSD_DEVICE_HPP_