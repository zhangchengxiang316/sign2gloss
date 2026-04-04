/*
 * @Author: Jingwen Bai
 * @Date: 2024-07-04 11:07:00
 * @Description: osd device
 * @Filename: osd-device.cpp
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <unistd.h>

#include "../include/osd-device.hpp"

using namespace fdevice;

namespace sst {
namespace device {
namespace osd {

OsdDevice::OsdDevice() : m_height(0), m_width(0) {}

OsdDevice::~OsdDevice() {
    std::cout << "OsdDevice Destructor" << std::endl;
}

void OsdDevice::Initialize(int width, int height) {
    m_width  = width;
    m_height = height;

    // Load OSD color LUT
    if (LoadLutFile(m_osd_lut_path.c_str()) != 0) {
        std::cerr << "[ERROR] OsdDevice: LoadLutFile failed: " << m_osd_lut_path << std::endl;
        return;
    }

    // Open device
    m_osd_handle = osd_open_device();
    if (m_osd_handle == INVALID_HANDLE) {
        std::cerr << "[ERROR] OsdDevice: osd_open_device failed!" << std::endl;
        return;
    }

    // Init device
    if (osd_init_device(m_osd_handle, OSD_LAYER_SIZE, (char*)m_pcolor_lut) != 0) {
        std::cerr << "[ERROR] OsdDevice: osd_init_device failed!" << std::endl;
        return;
    }

    // Init layers (all graphic quadrangle layers)
    int dma_size = 1024;
    for (int layer_index = 0; layer_index < OSD_LAYER_SIZE; layer_index++) {
        // allocate buffers (double buffer)
        osd_alloc_buffer(m_osd_handle, m_layer_dma[layer_index].dma, dma_size);
        usleep(250000); // 0.25s, per demo recommendation
        osd_alloc_buffer(m_osd_handle, m_layer_dma[layer_index].dma_2, dma_size);

        int dma_fd = osd_get_buffer_fd(m_osd_handle, m_layer_dma[layer_index].dma);
        if (dma_fd <= 0) {
            std::cerr << "[WARN] OsdDevice: osd_get_buffer_fd failed on layer " << layer_index << std::endl;
        }

        LAYER_ATTR_S osd_layer;
        std::memset(&osd_layer, 0, sizeof(osd_layer));

        osd_layer.codeTYPE = SS_TYPE_QUADRANGLE;
        osd_layer.layer_data_QR.osd_buf.buf_type = BUFFER_TYPE_DMABUF;
        osd_layer.layer_data_QR.osd_buf.buf.fd_dmabuf = dma_fd;

        osd_layer.layerStart.layer_start_x = 0;
        osd_layer.layerStart.layer_start_y = 0;

        osd_layer.layerSize.layer_width  = m_width;
        osd_layer.layerSize.layer_height = m_height;

        osd_layer.layer_rgn = {TYPE_GRAPHIC, {m_width, m_height}};

        if (osd_create_layer(m_osd_handle, (ssLAYER_HANDLE)layer_index, &osd_layer) != 0) {
            std::cerr << "[WARN] OsdDevice: osd_create_layer failed on layer " << layer_index << std::endl;
        }
        if (osd_set_layer_buffer(m_osd_handle, (ssLAYER_HANDLE)layer_index, m_layer_dma[layer_index]) != 0) {
            std::cerr << "[WARN] OsdDevice: osd_set_layer_buffer failed on layer " << layer_index << std::endl;
        }
    }
}

void OsdDevice::Release() {
    std::cout << "OsdDevice Release" << std::endl;

    if (m_osd_handle != INVALID_HANDLE) {
        // Destroy layers and buffers
        for (int i = 0; i < OSD_LAYER_SIZE; i++) {
            osd_destroy_layer(m_osd_handle, (ssLAYER_HANDLE)i);

            if (m_layer_dma[i].dma != nullptr) {
                osd_delete_buffer(m_osd_handle, m_layer_dma[i].dma);
                m_layer_dma[i].dma = nullptr;
            }
            if (m_layer_dma[i].dma_2 != nullptr) {
                osd_delete_buffer(m_osd_handle, m_layer_dma[i].dma_2);
                m_layer_dma[i].dma_2 = nullptr;
            }
        }

        osd_close_device(m_osd_handle);
        m_osd_handle = INVALID_HANDLE;
    }

    if (m_pcolor_lut != nullptr) {
        delete[] m_pcolor_lut;
        m_pcolor_lut = nullptr;
    }
}

void OsdDevice::ClearAll() {
    if (m_osd_handle != INVALID_HANDLE) {
        osd_clean_all_layer(m_osd_handle);
    }
}

void OsdDevice::ClearLayer(int layer_id) {
    if (m_osd_handle != INVALID_HANDLE) {
        osd_clean_layer(m_osd_handle, (ssLAYER_HANDLE)layer_id);
    }
}

int OsdDevice::LoadLutFile(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);
    if (!file) {
        std::cerr << "无法打开文件 " << filename << std::endl;
        return -1;
    }
    m_file_size = static_cast<int>(file.tellg());
    m_pcolor_lut = new uint8_t[m_file_size];
    file.seekg(0, std::ios::beg);
    file.read((char*)m_pcolor_lut, m_file_size);
    file.close();
    return 0;
}

// BOX-mode: create inner/outer rectangles from [xmin,ymin,xmax,ymax] with border thickness.
void OsdDevice::GenQrangleBox(std::array<float, 4>& det, int border,
                              fdevice::VERTEXS_S& out_v,
                              fdevice::VERTEXS_S& in_v) {
    // det = [xmin, ymin, xmax, ymax]
    int xmin = ClampX((int)det[0]);
    int ymin = ClampY((int)det[1]);
    int xmax = ClampX((int)det[2]);
    int ymax = ClampY((int)det[3]);

    // outer rect (expanded)
    int ox0 = ClampX(xmin - border);
    int oy0 = ClampY(ymin - border);
    int ox1 = ClampX(xmax + border);
    int oy1 = ClampY(ymax + border);

    // inner rect (shrunk)
    int ix0 = ClampX(xmin + border);
    int iy0 = ClampY(ymin + border);
    int ix1 = ClampX(xmax - border);
    int iy1 = ClampY(ymax - border);

    // Vertex order can be cw/ccw; keep consistent.
    out_v.points[0] = {ox0, oy0};
    out_v.points[1] = {ox0, oy1};
    out_v.points[2] = {ox1, oy1};
    out_v.points[3] = {ox1, oy0};

    in_v.points[0] = {ix0, iy0};
    in_v.points[1] = {ix0, iy1};
    in_v.points[2] = {ix1, iy1};
    in_v.points[3] = {ix1, iy0};
}

// draw mode: auto alloc layer (library chooses a suitable layer)
void OsdDevice::Draw(std::vector<OsdQuadRangle>& quad_rangle) {
    if (m_osd_handle == INVALID_HANDLE) return;

    if (quad_rangle.empty()) {
        osd_clean_all_layer(m_osd_handle);
        return;
    }

    for (auto& q : quad_rangle) {
        fdevice::VERTEXS_S out_v = {};
        fdevice::VERTEXS_S in_v  = {};

        if (q.shape_type == OsdShapeType::VERTEX) {
            out_v = q.vertex_out;
            in_v  = q.vertex_in;
        } else {
            // BOX mode fallback
            GenQrangleBox(q.box, q.border, out_v, in_v);
        }

        COVER_ATTR_S attr;
        std::memset(&attr, 0, sizeof(attr));
        attr.colorIdx    = q.color;
        attr.eSolid      = q.type;
        attr.alpha       = q.alpha;
        attr.vertex_out  = out_v;
        attr.vertex_in   = in_v;

        osd_add_quad_rangle(m_osd_handle, &attr);
    }

    osd_flush_quad_rangle(m_osd_handle);
}

// draw mode: manual alloc layer
void OsdDevice::Draw(std::vector<OsdQuadRangle>& quad_rangle, int layer_id) {
    if (m_osd_handle == INVALID_HANDLE) return;

    if (quad_rangle.empty()) {
        osd_clean_layer(m_osd_handle, (ssLAYER_HANDLE)layer_id);
        return;
    }

    for (auto& q : quad_rangle) {
        fdevice::VERTEXS_S out_v = {};
        fdevice::VERTEXS_S in_v  = {};

        if (q.shape_type == OsdShapeType::VERTEX) {
            out_v = q.vertex_out;
            in_v  = q.vertex_in;
        } else {
            GenQrangleBox(q.box, q.border, out_v, in_v);
        }

        COVER_ATTR_S attr;
        std::memset(&attr, 0, sizeof(attr));
        attr.colorIdx   = q.color;
        attr.eSolid     = q.type;
        attr.alpha      = q.alpha;
        attr.vertex_out = out_v;
        attr.vertex_in  = in_v;

        osd_add_quad_rangle_layer(m_osd_handle, (ssLAYER_HANDLE)layer_id, &attr);
    }

    osd_flush_quad_rangle_layer(m_osd_handle, (ssLAYER_HANDLE)layer_id);
}

// Legacy: manual box drawing from vector<boxes>
void OsdDevice::Draw(std::vector<std::array<float, 4>>& boxes,
                     int border,
                     int layer_id,
                     fdevice::QUADRANGLETYPE type,
                     fdevice::ALPHATYPE alpha,
                     int color) {
    if (m_osd_handle == INVALID_HANDLE) return;

    if (boxes.empty()) {
        osd_clean_layer(m_osd_handle, (ssLAYER_HANDLE)layer_id);
        return;
    }

    for (auto& box : boxes) {
        fdevice::VERTEXS_S out_v = {};
        fdevice::VERTEXS_S in_v  = {};
        GenQrangleBox(box, border, out_v, in_v);

        COVER_ATTR_S attr;
        std::memset(&attr, 0, sizeof(attr));
        attr.colorIdx   = color;
        attr.eSolid     = type;
        attr.alpha      = alpha;
        attr.vertex_out = out_v;
        attr.vertex_in  = in_v;

        osd_add_quad_rangle_layer(m_osd_handle, (ssLAYER_HANDLE)layer_id, &attr);
    }

    osd_flush_quad_rangle_layer(m_osd_handle, (ssLAYER_HANDLE)layer_id);
}

} // namespace osd
} // namespace device
} // namespace sst