#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include "BotanicalGarden.h"
#include "Tree.h"
#include "Bench.h"
#include "StreetLight.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <stdexcept>
#include <cwchar>

namespace
{
    // Native Windows text rasterization keeps Kazakh/Cyrillic lettering legible
    // without adding a font library, downloaded asset, or a new shader path.
    GLuint createEntranceSignTexture()
    {
        constexpr int width = 1024, rowHeight = 128, height = rowHeight * 3;
        struct TextSurface
        {
            HDC dc = nullptr;
            HBITMAP bitmap = nullptr;
            HFONT font = nullptr;
            HGDIOBJ oldBitmap = nullptr, oldFont = nullptr;
            ~TextSurface()
            {
                if (oldFont) SelectObject(dc, oldFont);
                if (oldBitmap) SelectObject(dc, oldBitmap);
                if (font) DeleteObject(font);
                if (bitmap) DeleteObject(bitmap);
                if (dc) DeleteDC(dc);
            }
        } surface;
        surface.dc = CreateCompatibleDC(nullptr);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height; // Top-down rows match the sign UVs.
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* pixels = nullptr;
        surface.bitmap = CreateDIBSection(surface.dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        surface.font = CreateFontW(-86, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        if (!surface.dc || !surface.bitmap || !surface.font)
            throw std::runtime_error("Cannot create Botanical Garden entrance lettering");
        surface.oldBitmap = SelectObject(surface.dc, surface.bitmap);
        surface.oldFont = SelectObject(surface.dc, surface.font);
        RECT bounds{0, 0, width, height};
        HBRUSH background = CreateSolidBrush(RGB(225, 222, 210));
        FillRect(surface.dc, &bounds, background);
        DeleteObject(background);
        SetBkMode(surface.dc, TRANSPARENT);
        SetTextColor(surface.dc, RGB(157, 111, 35));
        const wchar_t* names[] = {
            L"BOTANICAL GARDEN",
            L"\u0411\u041E\u0422\u0410\u041D\u0418\u041A\u0410\u041B\u042B\u049A \u0421\u0410\u042F\u0411\u0410\u049A",
            L"\u0411\u041E\u0422\u0410\u041D\u0418\u0427\u0415\u0421\u041A\u0418\u0419 \u0421\u0410\u0414"
        };
        for (int row = 0; row < 3; ++row)
        {
            int length = static_cast<int>(std::wcslen(names[row]));
            std::vector<WORD> glyphs(length);
            SIZE extent{};
            if (GetGlyphIndicesW(surface.dc, names[row], length, glyphs.data(),
                                 GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR ||
                std::find(glyphs.begin(), glyphs.end(), 0xFFFF) != glyphs.end() ||
                !GetTextExtentPoint32W(surface.dc, names[row], length, &extent) || extent.cx > width - 16)
                throw std::runtime_error("Entrance font cannot display the complete multilingual sign");
            RECT line{8, row * rowHeight, width - 8, (row + 1) * rowHeight};
            if (!DrawTextW(surface.dc, names[row], -1, &line, DT_CENTER | DT_VCENTER | DT_SINGLELINE))
                throw std::runtime_error("Cannot draw Botanical Garden entrance lettering");
        }
        GdiFlush();
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    constexpr float halfWidth = 105.0f;
    constexpr float halfDepth = 115.0f;
    constexpr float centralRadius = 8.0f;
    constexpr float pathHalfWidth = 2.0f;
    constexpr int circleSegments = 96;
    // In local space -Z lies to the visitor's left from the east entrance.
    constexpr Vec3 lakeLocalCenter{-48.0f, 0.0f, -78.0f};
    constexpr float lakeHalfLength = 40.0f;
    constexpr float greenhouseMinX = 30.0f, greenhouseMaxX = 56.0f;
    constexpr float greenhouseBack = -82.0f, greenhouseFront = -64.0f;
    constexpr float greenhouseEaves = 2.0f;
    struct GreenhouseHall { float left, right, peak; };
    constexpr GreenhouseHall greenhouseHalls[] = {
        {30.0f, 38.0f, 9.0f}, {38.0f, 48.0f, 11.0f}, {48.0f, 56.0f, 9.0f}
    };

    float hallRoofHeight(const GreenhouseHall& hall, float x)
    {
        float halfSpan = (hall.right - hall.left) * 0.5f;
        float middle = (hall.right + hall.left) * 0.5f;
        return greenhouseEaves + (hall.peak - greenhouseEaves) *
               (1.0f - std::abs(x - middle) / halfSpan);
    }

    float vestibuleRoofHeight(float x)
    {
        return 1.2f + 3.6f * (1.0f - std::abs(x - 43.0f) / 3.0f);
    }

    struct PavingOpening { float minX, maxX, minZ, maxZ; };
    // Existing outer trees remain in four lawn openings; four more openings
    // hold the new low flowerbeds. The axial spoke stays at its original height.
    constexpr PavingOpening entranceOpenings[] = {
        {-12.2f, -7.2f, 100.0f, 105.0f}, {7.2f, 12.2f, 100.0f, 105.0f},
        {-27.3f, -22.3f, 97.5f, 102.5f}, {22.3f, 27.3f, 97.5f, 102.5f},
        {-36.5f, -29.5f, 109, 111}, {-21.5f, -14.5f, 109, 111},
        {14.5f, 21.5f, 109, 111}, {29.5f, 36.5f, 109, 111},
        {-1.75f, 1.75f, 59, 108}
    };

    GLuint appendVertex(std::vector<float>& vertices, Vec3 point,
                        Vec3 normal = {0.0f, 1.0f, 0.0f})
    {
        GLuint index = static_cast<GLuint>(vertices.size() / 8);
        vertices.insert(vertices.end(), {
            point.x, point.y, point.z,
            normal.x, normal.y, normal.z,
            point.x / 12.0f, point.z / 12.0f
        });
        return index;
    }

    void appendFace(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal)
    {
        GLuint first = appendVertex(vertices, a, normal);
        appendVertex(vertices, b, normal);
        appendVertex(vertices, c, normal);
        appendVertex(vertices, d, normal);

        if (dot(cross(b - a, c - a), normal) > 0.0f)
        {
            indices.insert(indices.end(), {
                first, first + 1, first + 2,
                first + 1, first + 3, first + 2
            });
        }
        else
        {
            indices.insert(indices.end(), {
                first, first + 2, first + 1,
                first + 1, first + 2, first + 3
            });
        }
    }

    void appendQuad(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    Vec3 a, Vec3 b, Vec3 c, Vec3 d)
    {
        appendFace(vertices, indices, a, b, c, d, {0.0f, 1.0f, 0.0f});
    }

    void appendBox(std::vector<float>& vertices, std::vector<GLuint>& indices,
                   float minX, float maxX, float minY, float maxY,
                   float minZ, float maxZ)
    {
        appendFace(vertices, indices,
                   {minX, maxY, minZ}, {maxX, maxY, minZ},
                   {minX, maxY, maxZ}, {maxX, maxY, maxZ}, {0, 1, 0});
        appendFace(vertices, indices,
                   {minX, minY, minZ}, {maxX, minY, minZ},
                   {minX, minY, maxZ}, {maxX, minY, maxZ}, {0, -1, 0});
        appendFace(vertices, indices,
                   {minX, minY, minZ}, {maxX, minY, minZ},
                   {minX, maxY, minZ}, {maxX, maxY, minZ}, {0, 0, -1});
        appendFace(vertices, indices,
                   {minX, minY, maxZ}, {maxX, minY, maxZ},
                   {minX, maxY, maxZ}, {maxX, maxY, maxZ}, {0, 0, 1});
        appendFace(vertices, indices,
                   {minX, minY, minZ}, {minX, minY, maxZ},
                   {minX, maxY, minZ}, {minX, maxY, maxZ}, {-1, 0, 0});
        appendFace(vertices, indices,
                   {maxX, minY, minZ}, {maxX, minY, maxZ},
                   {maxX, maxY, minZ}, {maxX, maxY, maxZ}, {1, 0, 0});
    }

    void appendFurniturePart(std::vector<float>& vertices, std::vector<GLuint>& indices,
                             const Mat4& transform)
    {
        std::size_t first = vertices.size();
        appendBox(vertices, indices, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f);
        Vec3 axes[] = {{transform[0], transform[1], transform[2]},
                       {transform[4], transform[5], transform[6]},
                       {transform[8], transform[9], transform[10]}};
        for (std::size_t i = first; i < vertices.size(); i += 8)
        {
            Vec3 p = axes[0] * vertices[i] + axes[1] * vertices[i + 1]
                + axes[2] * vertices[i + 2] + Vec3{transform[12], transform[13], transform[14]};
            Vec3 n = normalize(axes[0] * (vertices[i + 3] / dot(axes[0], axes[0]))
                + axes[1] * (vertices[i + 4] / dot(axes[1], axes[1]))
                + axes[2] * (vertices[i + 5] / dot(axes[2], axes[2])));
            vertices[i] = p.x; vertices[i + 1] = p.y; vertices[i + 2] = p.z;
            vertices[i + 3] = n.x; vertices[i + 4] = n.y; vertices[i + 5] = n.z;
        }
    }

    void appendSteelBeam(std::vector<float>& vertices, std::vector<GLuint>& indices,
                         Vec3 start, Vec3 end, float width)
    {
        Vec3 direction = normalize(end - start);
        Vec3 reference = std::abs(direction.y) > 0.95f ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
        Vec3 right = normalize(cross(direction, reference));
        Vec3 up = normalize(cross(right, direction));
        Vec3 r = right * (width * 0.5f), u = up * (width * 0.5f);
        appendFace(vertices, indices, start - r - u, end - r - u,
                   start - r + u, end - r + u, right * -1.0f);
        appendFace(vertices, indices, start + r - u, end + r - u,
                   start + r + u, end + r + u, right);
        appendFace(vertices, indices, start - r + u, end - r + u,
                   start + r + u, end + r + u, up);
        appendFace(vertices, indices, start - r - u, end - r - u,
                   start + r - u, end + r - u, up * -1.0f);
        appendFace(vertices, indices, start - r - u, start + r - u,
                   start - r + u, start + r + u, direction * -1.0f);
        appendFace(vertices, indices, end - r - u, end + r - u,
                   end - r + u, end + r + u, direction);
    }

    void appendPlantEllipsoid(std::vector<float>& vertices,
                              std::vector<GLuint>& indices, Vec3 center, Vec3 radii)
    {
        constexpr int rings = 6;
        constexpr int segments = 10;
        GLuint first = static_cast<GLuint>(vertices.size() / 8);
        for (int ring = 0; ring <= rings; ++ring)
        {
            float latitude = pi * ring / rings;
            for (int segment = 0; segment <= segments; ++segment)
            {
                float longitude = 2.0f * pi * segment / segments;
                Vec3 unit{std::sin(latitude) * std::cos(longitude),
                          std::cos(latitude),
                          std::sin(latitude) * std::sin(longitude)};
                Vec3 normal = normalize(Vec3{unit.x / radii.x, unit.y / radii.y,
                                              unit.z / radii.z});
                appendVertex(vertices, center + Vec3{unit.x * radii.x,
                    unit.y * radii.y, unit.z * radii.z}, normal);
            }
        }
        for (int ring = 0; ring < rings; ++ring)
        {
            for (int segment = 0; segment < segments; ++segment)
            {
                GLuint a = first + ring * (segments + 1) + segment;
                GLuint b = a + segments + 1;
                if (ring != 0)
                {
                    indices.insert(indices.end(), {a, a + 1, b});
                }
                if (ring != rings - 1)
                {
                    indices.insert(indices.end(), {a + 1, b + 1, b});
                }
            }
        }
    }

    void appendConiferTier(std::vector<float>& vertices,
                           std::vector<GLuint>& indices, Vec3 base,
                           float radius, float height)
    {
        constexpr int segments = 10;
        for (int segment = 0; segment < segments; ++segment)
        {
            float a = 2.0f * pi * segment / segments;
            float b = 2.0f * pi * (segment + 1) / segments;
            Vec3 left = base + Vec3{radius * std::cos(a), 0, radius * std::sin(a)};
            Vec3 right = base + Vec3{radius * std::cos(b), 0, radius * std::sin(b)};
            Vec3 top = base + Vec3{0, height, 0};
            Vec3 normal = normalize(cross(top - left, right - left));
            GLuint first = appendVertex(vertices, left, normal);
            appendVertex(vertices, top, normal);
            appendVertex(vertices, right, normal);
            indices.insert(indices.end(), {first, first + 1, first + 2});
            first = appendVertex(vertices, base, {0, -1, 0});
            appendVertex(vertices, left, {0, -1, 0});
            appendVertex(vertices, right, {0, -1, 0});
            indices.insert(indices.end(), {first, first + 1, first + 2});
        }
    }

    // Folded, gently drooping leaves: three segments, both sides explicitly
    // modeled so the garden's normal back-face culling remains unchanged.
    void appendTropicalLeaf(std::vector<float>& vertices, std::vector<GLuint>& indices,
                            Vec3 root, float angle, float length, float width)
    {
        Vec3 direction{std::cos(angle), 0, std::sin(angle)};
        Vec3 side{-direction.z, 0, direction.x};
        const float widths[] = {0.015f, width, width * 0.65f, 0.003f};
        Vec3 left[4], spine[4], right[4];
        for (int i = 0; i <= 3; ++i)
        {
            float t = i / 3.0f;
            Vec3 center = root + direction * (length * t)
                + Vec3{0, length * (0.42f * std::sin(pi * t) - 0.20f * t), 0};
            left[i] = center - side * widths[i];
            right[i] = center + side * widths[i];
            spine[i] = center + Vec3{0, widths[i] * 0.25f, 0};
        }
        auto foldedFace = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d)
        {
            Vec3 normal = normalize(cross(b - a, c - a));
            appendFace(vertices, indices, a, b, c, d, normal);
            appendFace(vertices, indices, a, b, c, d, normal * -1.0f);
        };
        for (int i = 0; i < 3; ++i)
        {
            foldedFace(left[i], spine[i], left[i + 1], spine[i + 1]);
            foldedFace(spine[i], right[i], spine[i + 1], right[i + 1]);
        }
    }

    struct ConservatoryPlant
    {
        Vec3 root;
        float height;
        bool palm;
        float phase;
    };

    std::vector<ConservatoryPlant> conservatoryPlants()
    {
        std::vector<ConservatoryPlant> plants;
        // Two palms flank each of the six preserved broadleaf specimens.
        for (float x : {35.0f, 51.0f})
            for (float z : {-69.0f, -74.0f, -79.0f})
                for (float side : {-1.0f, 1.0f})
                    plants.push_back({{x + side * 0.55f, 0.56f, z + side * 0.25f},
                        side < 0 ? 2.75f : 3.20f, true, side * 0.4f});
        // Two low planting rows leave x=40.75..45.25 open as the main aisle.
        for (float x : {40.0f, 46.0f})
            for (int row = 0; row < 4; ++row)
                plants.push_back({{x, 0.49f, -68.0f - 4.0f * row},
                    0.60f + 0.10f * (row % 2), false, row * 0.35f});
        return plants;
    }

    Vec3 sectorPlantPosition(int sector, float radius, float fraction)
    {
        float angle = (sector + fraction) * pi / 4.0f;
        return {radius * std::cos(angle), 0.018f, radius * std::sin(angle)};
    }

    bool isOuterPlantingSite(Vec3 point, float footprintRadius)
    {
        float margin = footprintRadius + 1.0f;
        float radius = std::sqrt(point.x * point.x + point.z * point.z);
        if (radius <= 60.0f + margin ||
            std::abs(point.x) + margin >= halfWidth - 0.6f ||
            std::abs(point.z) + margin >= halfDepth - 0.6f)
        {
            return false;
        }
        // Local coordinates: retain the whole greenhouse reservation and a
        // generous envelope around the lake, shoreline walk, and both bridges.
        if ((point.x > 27.0f - margin && point.x < 87.0f + margin &&
             point.z > -84.0f - margin && point.z < -56.0f + margin) ||
            (point.x > -102.0f - margin && point.x < 7.0f + margin &&
             point.z > -97.0f - margin && point.z < -59.0f + margin))
        {
            return false;
        }
        // Keep the eight avenues open, including their sightlines beyond the
        // existing endpoints. The entrance axis is wider than the other rays.
        return std::abs(point.z) > 4.0f + margin &&
               std::abs(point.x) > 1.75f + margin &&
               std::abs(point.x - point.z) / std::sqrt(2.0f) > 1.75f + margin &&
               std::abs(point.x + point.z) / std::sqrt(2.0f) > 1.75f + margin;
    }

    void appendRectangle(std::vector<float>& vertices, std::vector<GLuint>& indices,
                         float minX, float maxX, float minZ, float maxZ, float height)
    {
        appendQuad(vertices, indices,
                   {minX, height, minZ}, {maxX, height, minZ},
                   {minX, height, maxZ}, {maxX, height, maxZ});
    }

    void appendEntranceCourt(std::vector<float>& vertices, std::vector<GLuint>& indices)
    {
        std::vector<float> xs{-40, 40}, zs{97, 114};
        for (const PavingOpening& opening : entranceOpenings)
        {
            xs.push_back(opening.minX); xs.push_back(opening.maxX);
            zs.push_back(std::clamp(opening.minZ, 97.0f, 114.0f));
            zs.push_back(std::clamp(opening.maxZ, 97.0f, 114.0f));
        }
        auto uniqueSorted = [](std::vector<float>& values)
        {
            std::sort(values.begin(), values.end());
            values.erase(std::unique(values.begin(), values.end()), values.end());
        };
        uniqueSorted(xs); uniqueSorted(zs);
        for (std::size_t x = 0; x + 1 < xs.size(); ++x)
            for (std::size_t z = 0; z + 1 < zs.size(); ++z)
            {
                float mx = (xs[x] + xs[x + 1]) * 0.5f, mz = (zs[z] + zs[z + 1]) * 0.5f;
                bool opening = false;
                for (const PavingOpening& hole : entranceOpenings)
                    if (mx > hole.minX && mx < hole.maxX && mz > hole.minZ && mz < hole.maxZ)
                        opening = true;
                if (!opening) appendRectangle(vertices, indices, xs[x], xs[x + 1], zs[z], zs[z + 1], 0.060f);
            }
        // Widen beside the existing spoke, never place another face on top of it.
        for (float side : {-1.0f, 1.0f})
        {
            GLuint first = appendVertex(vertices, {side * 1.75f, 0.060f, 72});
            appendVertex(vertices, {side * 1.75f, 0.060f, 82});
            appendVertex(vertices, {side * 4, 0.060f, 82});
            if (side > 0) indices.insert(indices.end(), {first, first + 1, first + 2});
            else indices.insert(indices.end(), {first, first + 2, first + 1});
            appendRectangle(vertices, indices, side < 0 ? -4.0f : 1.75f,
                            side < 0 ? -1.75f : 4.0f, 82, 97, 0.060f);
        }
        appendRectangle(vertices, indices, -4, 4, 114, 115, 0.060f);
    }

    void appendDisk(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    float radius, float height)
    {
        for (int segment = 0; segment < circleSegments; ++segment)
        {
            float firstAngle = 2.0f * pi * segment / circleSegments;
            float nextAngle = 2.0f * pi * (segment + 1) / circleSegments;
            GLuint first = appendVertex(vertices, {0, height, 0});
            appendVertex(vertices, {radius * std::cos(firstAngle), height,
                                    radius * std::sin(firstAngle)});
            appendVertex(vertices, {radius * std::cos(nextAngle), height,
                                    radius * std::sin(nextAngle)});
            indices.insert(indices.end(), {first, first + 2, first + 1});
        }
    }

    void appendRing(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    float innerRadius, float outerRadius, float height)
    {
        for (int segment = 0; segment < circleSegments; ++segment)
        {
            float firstAngle = 2.0f * pi * segment / circleSegments;
            float nextAngle = 2.0f * pi * (segment + 1) / circleSegments;
            appendQuad(vertices, indices,
                       {innerRadius * std::cos(firstAngle), height,
                        innerRadius * std::sin(firstAngle)},
                       {outerRadius * std::cos(firstAngle), height,
                        outerRadius * std::sin(firstAngle)},
                       {innerRadius * std::cos(nextAngle), height,
                        innerRadius * std::sin(nextAngle)},
                       {outerRadius * std::cos(nextAngle), height,
                        outerRadius * std::sin(nextAngle)});
        }
    }

    void appendSector(std::vector<float>& vertices, std::vector<GLuint>& indices,
                      int sector)
    {
        constexpr int radialSegments = 6;
        constexpr int arcSegments = 16;
        float sectorStart = 2.0f * pi * sector / 8.0f;
        float sectorEnd = 2.0f * pi * (sector + 1) / 8.0f;

        auto point = [sectorStart, sectorEnd](float radial, float fraction)
        {
            // Eight green points taper toward the spokes, with a clear gap
            // for each four-meter pedestrian route.
            float pointShape = 1.0f - std::abs(2.0f * fraction - 1.0f);
            float tipRadius = 27.0f + 16.0f * std::pow(pointShape, 1.35f);
            float radius = centralRadius + (tipRadius - centralRadius) * radial;
            float clearance = std::asin(pathHalfWidth / radius);
            float angle = sectorStart + clearance +
                fraction * (sectorEnd - sectorStart - 2.0f * clearance);
            return Vec3{radius * std::cos(angle), 0.037f,
                        radius * std::sin(angle)};
        };

        for (int ring = 0; ring < radialSegments; ++ring)
        {
            float inner = static_cast<float>(ring) / radialSegments;
            float outer = static_cast<float>(ring + 1) / radialSegments;

            for (int arc = 0; arc < arcSegments; ++arc)
            {
                float start = static_cast<float>(arc) / arcSegments;
                float end = static_cast<float>(arc + 1) / arcSegments;
                appendQuad(vertices, indices, point(inner, start),
                           point(outer, start), point(inner, end),
                           point(outer, end));
            }
        }
    }

    void appendSectorBand(std::vector<float>& vertices,
                          std::vector<GLuint>& indices, int sector,
                          float innerRadius, float outerRadius, float height)
    {
        constexpr int segments = 16;
        float sectorStart = 2.0f * pi * sector / 8.0f;
        float sectorEnd = 2.0f * pi * (sector + 1) / 8.0f;
        auto point = [sectorStart, sectorEnd, height](float radius, float fraction)
        {
            float clearance = std::asin(pathHalfWidth / radius);
            float angle = sectorStart + clearance +
                (0.18f + 0.64f * fraction) *
                (sectorEnd - sectorStart - 2.0f * clearance);
            return Vec3{radius * std::cos(angle), height,
                        radius * std::sin(angle)};
        };

        for (int segment = 0; segment < segments; ++segment)
        {
            float start = static_cast<float>(segment) / segments;
            float end = static_cast<float>(segment + 1) / segments;
            appendQuad(vertices, indices, point(innerRadius, start),
                       point(outerRadius, start), point(innerRadius, end),
                       point(outerRadius, end));
        }
    }

    void appendRadialPath(std::vector<float>& vertices,
                          std::vector<GLuint>& indices, int path)
    {
        float angle = 2.0f * pi * path / 8.0f;
        float forwardX = std::cos(angle);
        float forwardZ = std::sin(angle);
        float sideX = -forwardZ * pathHalfWidth;
        float sideZ = forwardX * pathHalfWidth;
        constexpr float inner = centralRadius - 0.1f;
        constexpr float outer = 58.0f;
        constexpr float height = 0.06f;

        appendQuad(vertices, indices,
                   {inner * forwardX - sideX, height,
                    inner * forwardZ - sideZ},
                   {outer * forwardX - sideX, height,
                    outer * forwardZ - sideZ},
                   {inner * forwardX + sideX, height,
                    inner * forwardZ + sideZ},
                   {outer * forwardX + sideX, height,
                    outer * forwardZ + sideZ});
    }

    void appendOutwardPath(std::vector<float>& vertices,
                           std::vector<GLuint>& indices, Vec3 endpoint)
    {
        Vec3 direction = normalize(endpoint);
        Vec3 side{-direction.z * 1.75f, 0.0f, direction.x * 1.75f};
        // The ring covers this short junction overlap at its existing height.
        Vec3 start = direction * 59.0f;
        start.y = 0.060f;
        endpoint.y = 0.060f;
        appendQuad(vertices, indices, start - side, endpoint - side,
                   start + side, endpoint + side);
    }

    void appendOrnamentalArc(std::vector<float>& vertices,
                             std::vector<GLuint>& indices, int sector,
                             float radiusAtSpoke, float inwardBow)
    {
        constexpr int segments = 24;
        constexpr float halfPathWidth = 1.0f;
        float startAngle = 2.0f * pi * sector / 8.0f;
        float sweep = 2.0f * pi / 8.0f;

        auto edge = [startAngle, sweep, halfPathWidth,
                     radiusAtSpoke, inwardBow](float fraction, float side)
        {
            float angle = startAngle + sweep * fraction;
            // The arc bows toward the center, ending on both radial avenues.
            float radius = radiusAtSpoke - inwardBow * std::sin(pi * fraction) +
                           side * halfPathWidth;
            return Vec3{radius * std::cos(angle), 0.064f,
                        radius * std::sin(angle)};
        };

        for (int segment = 0; segment < segments; ++segment)
        {
            float first = static_cast<float>(segment) / segments;
            float next = static_cast<float>(segment + 1) / segments;
            appendQuad(vertices, indices, edge(first, -1.0f),
                       edge(first, 1.0f), edge(next, -1.0f),
                       edge(next, 1.0f));
        }
    }

    float lakeHalfWidth(float longitudinal)
    {
        float x = lakeLocalCenter.x + lakeHalfLength * longitudinal;
        auto lobe = [x](float center, float spread)
        {
            float distance = (x - center) / spread;
            return std::exp(-distance * distance);
        };
        return 3.0f + 13.0f * lobe(-70.0f, 9.0f)
                    + 9.0f * lobe(-45.0f, 7.0f)
                    + 12.0f * lobe(-18.0f, 8.0f);
    }

    Vec3 lakePoint(float angle, float scale, float height)
    {
        // Three broad lobes and two narrow waists form one connected lake.
        float longitudinal = std::cos(angle);
        float width = lakeHalfWidth(longitudinal);
        return {
            lakeLocalCenter.x + lakeHalfLength * scale * longitudinal,
            height,
            lakeLocalCenter.z + width * scale * std::sin(angle)
        };
    }

    void appendLakeBand(std::vector<float>& vertices,
                        std::vector<GLuint>& indices, float innerScale,
                        float outerScale, float innerHeight, float outerHeight,
                        float startAngle = 0.0f,
                        float endAngle = 2.0f * pi,
                        int segments = circleSegments)
    {
        float slope = (outerHeight - innerHeight) / (outerScale - innerScale);
        auto surfaceNormal = [slope](float angle, float scale, float height)
        {
            constexpr float delta = 0.001f;
            Vec3 tangent = lakePoint(angle + delta, scale, height) -
                           lakePoint(angle - delta, scale, height);
            Vec3 outward = lakePoint(angle, scale + delta,
                                     height + slope * delta) -
                           lakePoint(angle, scale - delta,
                                     height - slope * delta);
            return normalize(cross(tangent, outward));
        };

        for (int segment = 0; segment < segments; ++segment)
        {
            float start = startAngle +
                (endAngle - startAngle) * segment / segments;
            float end = startAngle +
                (endAngle - startAngle) * (segment + 1) / segments;
            Vec3 a = lakePoint(start, innerScale, innerHeight);
            Vec3 b = lakePoint(start, outerScale, outerHeight);
            Vec3 c = lakePoint(end, innerScale, innerHeight);
            Vec3 d = lakePoint(end, outerScale, outerHeight);
            GLuint first = appendVertex(vertices, a,
                                        surfaceNormal(start, innerScale, innerHeight));
            appendVertex(vertices, b,
                         surfaceNormal(start, outerScale, outerHeight));
            appendVertex(vertices, c,
                         surfaceNormal(end, innerScale, innerHeight));
            appendVertex(vertices, d,
                         surfaceNormal(end, outerScale, outerHeight));

            if (cross(b - a, c - a).y > 0.0f)
            {
                indices.insert(indices.end(), {
                    first, first + 1, first + 2,
                    first + 1, first + 3, first + 2
                });
            }
            else
            {
                indices.insert(indices.end(), {
                    first, first + 2, first + 1,
                    first + 1, first + 2, first + 3
                });
            }
        }
    }

    void appendLakeWater(std::vector<float>& vertices,
                         std::vector<GLuint>& indices)
    {
        constexpr float waterHeight = 0.034f;
        for (int segment = 0; segment < circleSegments; ++segment)
        {
            float firstX = -1.0f + 2.0f * segment / circleSegments;
            float nextX = -1.0f + 2.0f * (segment + 1) / circleSegments;
            float firstAngle = std::acos(firstX);
            float nextAngle = std::acos(nextX);
            appendQuad(vertices, indices,
                       lakePoint(-firstAngle, 1.0f, waterHeight),
                       lakePoint(-nextAngle, 1.0f, waterHeight),
                       lakePoint(firstAngle, 1.0f, waterHeight),
                       lakePoint(nextAngle, 1.0f, waterHeight));
        }
    }

    void appendLakeConnector(std::vector<float>& vertices,
                             std::vector<GLuint>& indices)
    {
        // A short spur joins the existing circular walk to the shore path.
        Vec3 shore = lakePoint(0.25f * pi, 1.28f, 0.054f);
        Vec3 direction = normalize(Vec3{shore.x, 0.0f, shore.z});
        Vec3 side{-direction.z * 1.3f, 0.0f, direction.x * 1.3f};
        Vec3 start = direction * 59.0f;
        start.y = 0.054f;
        appendQuad(vertices, indices, start - side, shore - side,
                   start + side, shore + side);
    }

    float lakeEdgeAtX(float x, float scale, bool upper)
    {
        float longitudinal = std::clamp(
            (x - lakeLocalCenter.x) / (lakeHalfLength * scale), -1.0f, 1.0f);
        float angle = std::acos(longitudinal);
        return lakePoint(upper ? angle : -angle, scale, 0.0f).z;
    }

    Vec3 lakeOutward(float angle)
    {
        Vec3 tangent = lakePoint(angle + 0.001f, 1.33f, 0) - lakePoint(angle - 0.001f, 1.33f, 0);
        return normalize(Vec3{tangent.z, 0, -tangent.x});
    }

    struct LakeBay { int firstSegment, segments; float depth; bool bench; };
    // Bays extend from exact vertices of the existing 96-segment promenade.
    const LakeBay lakeBays[] = {
        {13, 2, 2.2f, true}, {23, 2, 2.2f, true},
        {35, 2, 2.2f, true}, {83, 2, 2.2f, true},
        {17, 1, 0.9f, false}, {21, 1, 0.9f, false},
        {33, 1, 0.9f, false}, {56, 1, 0.9f, false},
        {69, 1, 0.9f, false}, {79, 1, 0.9f, false}
    };

    Vec3 lakeBayPosition(const LakeBay& bay)
    {
        float angle = (bay.firstSegment + bay.segments * 0.5f) * 2.0f * pi / circleSegments;
        // Linear midpoint for a one-segment bay matches the actual path edge.
        Vec3 edge = lakePoint(angle, 1.33f, 0.052f);
        if (bay.segments == 1)
            edge = (lakePoint(angle - pi / circleSegments, 1.33f, 0.052f)
                  + lakePoint(angle + pi / circleSegments, 1.33f, 0.052f)) * 0.5f;
        return edge + lakeOutward(angle) * (bay.bench ? 1.30f : 0.45f);
    }

    float lakeBayRotation(const LakeBay& bay)
    {
        Vec3 normal = lakeOutward((bay.firstSegment + bay.segments * 0.5f)
                                 * 2.0f * pi / circleSegments);
        return std::atan2(normal.x, normal.z) * 180.0f / pi;
    }

    float segmentDistance(Vec3 p, Vec3 a, Vec3 b)
    {
        p.y = a.y = b.y = 0;
        Vec3 d = b - a;
        float t = std::clamp(dot(p - a, d) / dot(d, d), 0.0f, 1.0f);
        Vec3 delta = p - (a + d * t);
        return std::sqrt(dot(delta, delta));
    }

    bool clearLakePlantingSite(Vec3 p, float radius)
    {
        if (std::abs(p.x) + radius > halfWidth - 1 ||
            std::abs(p.z) + radius > halfDepth - 1 ||
            std::sqrt(p.x * p.x + p.z * p.z) < 61 + radius) return false;
        // Exclude the entire lake and promenade polygon, plus its edge clearance.
        bool inside = false;
        for (int i = 0; i < circleSegments; ++i)
        {
            Vec3 a = lakePoint(i * 2.0f * pi / circleSegments, 1.33f, 0);
            Vec3 b = lakePoint((i + 1) * 2.0f * pi / circleSegments, 1.33f, 0);
            if (segmentDistance(p, a, b) < radius + 0.35f) return false;
            if ((a.z > p.z) != (b.z > p.z) &&
                p.x < (b.x - a.x) * (p.z - a.z) / (b.z - a.z) + a.x) inside = !inside;
        }
        if (inside) return false;
        for (float x : {-58.0f, -31.0f})
            if (segmentDistance(p, {x, 0, lakeEdgeAtX(x, 1.28f, false)},
                                   {x, 0, lakeEdgeAtX(x, 1.28f, true)}) < 2.1f + radius) return false;
        const Vec3 rayEnds[] = {{-62, 0, -62}, {0, 0, -69}, {96, 0, -96}};
        for (Vec3 end : rayEnds)
            if (segmentDistance(p, {0, 0, 0}, end) < 2.1f + radius) return false;
        Vec3 shore = lakePoint(pi * 0.25f, 1.28f, 0);
        Vec3 start = normalize(Vec3{shore.x, 0, shore.z}) * 59.0f;
        if (segmentDistance(p, start, shore) < 1.65f + radius) return false;
        for (const LakeBay& bay : lakeBays)
        {
            Vec3 delta = p - lakeBayPosition(bay); delta.y = 0;
            if (std::sqrt(dot(delta, delta)) < radius + (bay.bench ? 2.5f : 2.0f)) return false;
        }
        // Preserve the existing rear flower strip, hedge row and tree row.
        return p.z - radius > -99.2f;
    }

    void appendBridgeDeck(std::vector<float>& vertices,
                          std::vector<GLuint>& indices, float x)
    {
        float lowerWater = lakeEdgeAtX(x, 1.0f, false);
        float upperWater = lakeEdgeAtX(x, 1.0f, true);
        float lowerPath = lakeEdgeAtX(x, 1.28f, false);
        float upperPath = lakeEdgeAtX(x, 1.28f, true);
        constexpr float bridgeHalfWidth = 1.6f;
        constexpr float deckTop = 0.15f;
        constexpr float pathTop = 0.052f;
        float deckStart = lowerWater - 0.25f;
        float deckEnd = upperWater + 0.25f;

        appendBox(vertices, indices, x - bridgeHalfWidth, x + bridgeHalfWidth,
                  0.035f, deckTop, deckStart, deckEnd);
        appendQuad(vertices, indices,
                   {x - bridgeHalfWidth, pathTop, lowerPath},
                   {x + bridgeHalfWidth, pathTop, lowerPath},
                   {x - bridgeHalfWidth, deckTop, deckStart},
                   {x + bridgeHalfWidth, deckTop, deckStart});
        appendQuad(vertices, indices,
                   {x - bridgeHalfWidth, deckTop, deckEnd},
                   {x + bridgeHalfWidth, deckTop, deckEnd},
                   {x - bridgeHalfWidth, pathTop, upperPath},
                   {x + bridgeHalfWidth, pathTop, upperPath});
    }

    void appendBridgeRails(std::vector<float>& vertices,
                           std::vector<GLuint>& indices, float x)
    {
        const float lower = lakeEdgeAtX(x, 1.28f, false);
        const float start = lakeEdgeAtX(x, 1.0f, false) - 0.25f;
        const float end = lakeEdgeAtX(x, 1.0f, true) + 0.25f;
        const float upper = lakeEdgeAtX(x, 1.28f, true);
        const float levels[] = {lower, start, end, upper};
        const float heights[] = {0.052f, 0.15f, 0.15f, 0.052f};
        for (float side : {-1.0f, 1.0f})
        {
            float railX = x + side * 1.50f;
            for (int section = 0; section < 3; ++section)
            {
                for (float height : {0.52f, 1.05f})
                    appendSteelBeam(vertices, indices,
                        {railX, heights[section] + height, levels[section]},
                        {railX, heights[section + 1] + height, levels[section + 1]},
                        height > 1 ? 0.055f : 0.035f);
                int posts = static_cast<int>(std::ceil((levels[section + 1] - levels[section]) / 0.9f));
                for (int post = 0; post < posts; ++post)
                {
                    float t = static_cast<float>(post) / posts;
                    float z = levels[section] + (levels[section + 1] - levels[section]) * t;
                    float y = heights[section] + (heights[section + 1] - heights[section]) * t;
                    appendSteelBeam(vertices, indices, {railX, y, z}, {railX, y + 1.05f, z}, 0.045f);
                }
            }
            appendSteelBeam(vertices, indices, {railX, 0.052f, upper},
                            {railX, 1.102f, upper}, 0.045f);
        }
    }
}

BotanicalGarden::BotanicalGarden()
    : model(modelMatrix(center, 180.0f, {1.0f, 1.0f, 1.0f}))
{
    std::vector<float> vertices;
    std::vector<GLuint> indices;

    auto finishRange = [&indices](IndexRange& range)
    {
        range.count = static_cast<GLsizei>(indices.size() - range.firstIndex);
    };

    district.firstIndex = indices.size();
    // The open northern area (x = -457..-397, z = -74..-46)
    // remains reserved for the future greenhouse.
    appendRectangle(vertices, indices, -halfWidth, halfWidth,
                    -halfDepth, halfDepth, 0.018f);
    finishRange(district);

    border.firstIndex = indices.size();
    constexpr float borderWidth = 0.6f;
    appendRectangle(vertices, indices, -halfWidth, -halfWidth + borderWidth,
                    -halfDepth, -6.0f, 0.027f);
    appendRectangle(vertices, indices, -halfWidth, -halfWidth + borderWidth,
                    6.0f, halfDepth, 0.027f);
    appendRectangle(vertices, indices, halfWidth - borderWidth, halfWidth,
                    -halfDepth, halfDepth, 0.027f);
    appendRectangle(vertices, indices, -halfWidth + borderWidth,
                    halfWidth - borderWidth, -halfDepth,
                    -halfDepth + borderWidth, 0.027f);
    // Open only the southern boundary border for the new eight-meter entrance.
    appendRectangle(vertices, indices, -halfWidth + borderWidth,
                    -4, halfDepth - borderWidth, halfDepth, 0.027f);
    appendRectangle(vertices, indices, 4,
                    halfWidth - borderWidth, halfDepth - borderWidth, halfDepth, 0.027f);
    finishRange(border);

    lightSectors.firstIndex = indices.size();
    for (int sector = 0; sector < 8; sector += 2)
    {
        appendSector(vertices, indices, sector);
    }
    finishRange(lightSectors);

    darkSectors.firstIndex = indices.size();
    for (int sector = 1; sector < 8; sector += 2)
    {
        appendSector(vertices, indices, sector);
    }
    finishRange(darkSectors);

    // Four alternating points contain low, geometric flowerbed zones.
    flowerSoil.firstIndex = indices.size();
    for (int sector = 0; sector < 8; sector += 2)
    {
        appendSectorBand(vertices, indices, sector, 16.0f, 30.0f, 0.043f);
    }
    finishRange(flowerSoil);

    flowerRed.firstIndex = indices.size();
    for (int sector = 0; sector < 8; sector += 2)
    {
        appendSectorBand(vertices, indices, sector, 17.0f, 20.5f, 0.048f);
    }
    finishRange(flowerRed);

    flowerGold.firstIndex = indices.size();
    for (int sector = 0; sector < 8; sector += 2)
    {
        appendSectorBand(vertices, indices, sector, 22.0f, 25.0f, 0.048f);
    }
    finishRange(flowerGold);

    flowerPurple.firstIndex = indices.size();
    for (int sector = 0; sector < 8; sector += 2)
    {
        appendSectorBand(vertices, indices, sector, 27.0f, 29.0f, 0.048f);
    }
    finishRange(flowerPurple);

    radialPaths.firstIndex = indices.size();
    for (int path = 0; path < 8; ++path)
    {
        if (path != 0 && path != 4)
        {
            appendRadialPath(vertices, indices, path);
        }
    }
    finishRange(radialPaths);

    outwardPaths.firstIndex = indices.size();
    // The landmark rotates local X/Z by 180 degrees. List endpoints in local
    // space with their world X/Z destinations; the east entrance is the eighth ray.
    const Vec3 outwardEndpoints[] = {
        {-96.0f, 0.0f, 96.0f}, // (-274, -226): restore the approved endpoint.
        {0.0f, 0.0f, 108.0f},  // (-370, -238).
        {96.0f, 0.0f, 96.0f},  // (-466, -226).
        {98.0f, 0.0f, 0.0f},   // (-468, -130).
        {96.0f, 0.0f, -96.0f}, // (-466, -34): complete the short west diagonal.
        {0.0f, 0.0f, -69.0f},  // (-370, -61): stop before the shoreline walk.
        {-62.0f, 0.0f, -62.0f} // (-308, -68): preserve the approved lake-side ray.
    };
    for (Vec3 endpoint : outwardEndpoints)
    {
        appendOutwardPath(vertices, indices, endpoint);
    }
    finishRange(outwardPaths);

    mainAxis.firstIndex = indices.size();
    // One broad, uninterrupted east-west avenue crosses the central plaza.
    appendRectangle(vertices, indices, -halfWidth, 58.0f,
                    -4.0f, 4.0f, 0.062f);
    finishRange(mainAxis);

    innerCurvedPath.firstIndex = indices.size();
    appendRing(vertices, indices, 12.5f, 15.5f, 0.068f);
    finishRange(innerCurvedPath);

    middleCurvedPath.firstIndex = indices.size();
    appendRing(vertices, indices, 31.3f, 34.3f, 0.070f);
    finishRange(middleCurvedPath);

    ornamentalPaths.firstIndex = indices.size();
    for (int sector = 0; sector < 8; ++sector)
    {
        appendOrnamentalArc(vertices, indices, sector, 39.0f, 2.5f);
        appendOrnamentalArc(vertices, indices, sector, 49.0f, 3.0f);
    }
    finishRange(ornamentalPaths);

    circularPath.firstIndex = indices.size();
    appendRing(vertices, indices, 55.0f, 60.0f, 0.073f);
    finishRange(circularPath);

    centralPlaza.firstIndex = indices.size();
    appendDisk(vertices, indices, centralRadius + 0.2f, 0.078f);
    finishRange(centralPlaza);

    lakeGrassBank.firstIndex = indices.size();
    appendLakeBand(vertices, indices, 1.09f, 1.23f, 0.045f, 0.018f);
    finishRange(lakeGrassBank);

    lakeSoilBank.firstIndex = indices.size();
    appendLakeBand(vertices, indices, 1.0f, 1.09f, 0.034f, 0.045f);
    finishRange(lakeSoilBank);

    lakeShorePath.firstIndex = indices.size();
    appendLakeBand(vertices, indices, 1.24f, 1.33f, 0.052f, 0.052f);
    appendLakeConnector(vertices, indices);
    // Small flush pull-outs keep the narrow shore promenade unobstructed.
    for (const LakeBay& bay : lakeBays)
        for (int segment = bay.firstSegment; segment < bay.firstSegment + bay.segments; ++segment)
        {
            float a = segment * 2.0f * pi / circleSegments;
            float b = (segment + 1) * 2.0f * pi / circleSegments;
            Vec3 p = lakePoint(a, 1.33f, 0.052f), q = lakePoint(b, 1.33f, 0.052f);
            appendQuad(vertices, indices, p, q, p + lakeOutward(a) * bay.depth,
                       q + lakeOutward(b) * bay.depth);
        }
    finishRange(lakeShorePath);

    lakeWater.firstIndex = indices.size();
    appendLakeWater(vertices, indices);
    finishRange(lakeWater);

    bridgeDecks.firstIndex = indices.size();
    // A narrow beveled stone edge softens the promenade-to-lawn transition.
    // Leave the new seating and lamp bay openings flush and unobstructed.
    for (int segment = 0; segment < circleSegments; ++segment)
    {
        bool bayOpening = false;
        for (const LakeBay& bay : lakeBays)
            if (segment >= bay.firstSegment && segment < bay.firstSegment + bay.segments) bayOpening = true;
        if (!bayOpening)
            appendLakeBand(vertices, indices, 1.33f, 1.345f, 0.052f, 0.018f,
                           segment * 2.0f * pi / circleSegments,
                           (segment + 1) * 2.0f * pi / circleSegments, 1);
    }
    for (float x : {-58.0f, -31.0f})
    {
        appendBridgeDeck(vertices, indices, x);
    }
    finishRange(bridgeDecks);

    bridgeRails.firstIndex = indices.size();
    for (float x : {-58.0f, -31.0f})
    {
        appendBridgeRails(vertices, indices, x);
    }
    finishRange(bridgeRails);

    // Bake the existing furniture components into material batches.
    // Lamps use 60% scale (3.82 m tall), and remain unlit in daylight.
    for (const LakeBay& bay : lakeBays)
    {
        if (bay.bench) continue;
        StreetLight light(lakeBayPosition(bay), lakeBayRotation(bay), 0.60f);
        for (const Mat4& part : light.metalTransforms()) appendFurniturePart(vertices, indices, part);
    }
    finishRange(bridgeRails);

    lakesideBenchFrames.firstIndex = indices.size();
    for (const LakeBay& bay : lakeBays)
    {
        if (!bay.bench) continue;
        Bench bench(lakeBayPosition(bay), lakeBayRotation(bay));
        for (const Mat4& part : bench.frameTransforms()) appendFurniturePart(vertices, indices, part);
    }
    finishRange(lakesideBenchFrames);
    lakesideWood.firstIndex = indices.size();
    for (const LakeBay& bay : lakeBays)
    {
        if (!bay.bench) continue;
        Bench bench(lakeBayPosition(bay), lakeBayRotation(bay));
        for (const Mat4& part : bench.slatTransforms()) appendFurniturePart(vertices, indices, part);
    }
    finishRange(lakesideWood);
    lakesideDiffusers.firstIndex = indices.size();
    for (const LakeBay& bay : lakeBays)
    {
        if (bay.bench) continue;
        StreetLight light(lakeBayPosition(bay), lakeBayRotation(bay), 0.60f);
        appendFurniturePart(vertices, indices, light.diffuserTransform());
    }
    finishRange(lakesideDiffusers);

    struct GardenTree
    {
        Vec3 root;
        float scale;
        int kind; // 0: existing clustered crown, 1: columnar, 2: conifer.
    };
    std::vector<GardenTree> trees;
    trees.reserve(240);
    for (int sector = 0; sector < 8; ++sector)
    {
        for (int column = 0; column < 5; ++column)
        {
            trees.push_back({sectorPlantPosition(sector, 42.5f,
                0.18f + 0.16f * column), 0.48f + 0.01f * (column % 3), 0});
        }
        for (int column = 0; column < 7; ++column)
        {
            trees.push_back({sectorPlantPosition(sector, 52.5f,
                0.16f + (0.68f / 6.0f) * column),
                0.94f + 0.03f * (column % 3), column % 2 == 0 ? 1 : 2});
        }
    }

    // Outer groves: regular rows with a distinct dominant species per sector.
    // Filtering removes whole planting sites, never changes any approved path.
    for (int sector = 0; sector < 8; ++sector)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 5; ++column)
            {
                int kind = sector % 3;
                float scale = kind == 0 ? 0.60f + 0.04f * row : 0.90f + 0.05f * row;
                scale += 0.02f * (column % 2);
                float crownRadius = (kind == 0 ? 2.9f : kind == 1 ? 1.45f : 1.55f) * scale;
                Vec3 root = sectorPlantPosition(sector, 70.0f + 11.0f * row,
                                                0.12f + 0.19f * column);
                if (isOuterPlantingSite(root, crownRadius))
                {
                    trees.push_back({root, scale, kind});
                }
            }
        }
    }
    // A spaced columnar row behind the water leaves the shore promenade clear.
    for (float x = -84.0f; x <= 0.0f; x += 12.0f)
    {
        Vec3 root{x, 0.018f, -106.0f};
        if (isOuterPlantingSite(root, 1.45f))
        {
            trees.push_back({root, 1.0f, 1});
        }
    }

    auto clearOfTreeCrowns = [&trees](Vec3 point, float footprintRadius)
    {
        for (const GardenTree& tree : trees)
        {
            float crownRadius = (tree.kind == 0 ? 2.9f : tree.kind == 1 ? 1.45f : 1.55f) * tree.scale;
            float dx = point.x - tree.root.x;
            float dz = point.z - tree.root.z;
            float separation = crownRadius + footprintRadius + 0.6f;
            if (dx * dx + dz * dz < separation * separation) return false;
        }
        return true;
    };

    // Small trees are placed outside the shore promenade, with open gaps
    // between their crowns. Existing tree/flower placement remains untouched.
    std::vector<GardenTree> lakeTrees;
    for (float degrees : {48.0f, 70.0f, 85.0f, 100.0f, 130.0f, 145.0f,
                          205.0f, 225.0f, 245.0f, 270.0f, 300.0f, 325.0f})
    {
        float angle = degrees * pi / 180.0f;
        Vec3 p = lakePoint(angle, 1.33f, 0.018f) + lakeOutward(angle) * 4.8f;
        if (!clearLakePlantingSite(p, 1.5f) || !clearOfTreeCrowns(p, 1.5f)) continue;
        bool separated = true;
        for (const GardenTree& tree : lakeTrees)
        {
            Vec3 d = p - tree.root;
            if (dot(d, d) < 25.0f) separated = false;
        }
        if (separated && lakeTrees.size() < 8)
            lakeTrees.push_back({p, 0.48f + 0.02f * (lakeTrees.size() % 3), 0});
    }
    std::vector<Vec3> lakeShrubs, lakeGrasses, lakeFlowers;
    for (int group = 0; group < 24; ++group)
    {
        float angle = (group * 4.0f + 1.0f) * 2.0f * pi / circleSegments;
        Vec3 normal = lakeOutward(angle);
        Vec3 tangent{-normal.z, 0, normal.x};
        Vec3 groupCenter = lakePoint(angle, 1.33f, 0.018f) + normal * 1.45f;
        int count = group % 3 == 0 ? 3 : 5;
        for (int member = 0; member < count; ++member)
        {
            float radius = group % 3 == 0 ? 0.38f : 0.26f;
            Vec3 p = groupCenter + tangent * ((member - (count - 1) * 0.5f) * (radius * 2.2f));
            if (!clearLakePlantingSite(p, radius) || !clearOfTreeCrowns(p, radius)) continue;
            bool separated = true;
            for (const GardenTree& tree : lakeTrees)
            {
                Vec3 d = p - tree.root;
                if (dot(d, d) < 4.0f) separated = false;
            }
            if (!separated) continue;
            if (group % 3 == 0) lakeShrubs.push_back(p);
            else if (group % 3 == 1) lakeGrasses.push_back(p);
            else lakeFlowers.push_back(p);
        }
    }

    treeTrunks.firstIndex = indices.size();
    for (const GardenTree& tree : trees)
    {
        float radius = (tree.kind == 0 ? 0.325f : 0.19f) * tree.scale;
        float height = (tree.kind == 0 ? 4.4f : 3.8f) * tree.scale;
        appendBox(vertices, indices, tree.root.x - radius, tree.root.x + radius,
                  tree.root.y, tree.root.y + height,
                  tree.root.z - radius, tree.root.z + radius);
    }
    for (const GardenTree& tree : lakeTrees)
        appendBox(vertices, indices, tree.root.x - 0.12f, tree.root.x + 0.12f,
                  tree.root.y, tree.root.y + 2.2f, tree.root.z - 0.12f, tree.root.z + 0.12f);
    finishRange(treeTrunks);

    clusteredFoliage.firstIndex = indices.size();
    for (const GardenTree& tree : trees)
    {
        if (tree.kind != 0) continue;
        for (std::size_t cluster = 0; cluster < Tree::foliageOffsets.size(); ++cluster)
        {
            float radius = Tree::foliageRadii[cluster] * tree.scale;
            appendPlantEllipsoid(vertices, indices,
                tree.root + Tree::foliageOffsets[cluster] * tree.scale,
                {radius, radius, radius});
        }
    }
    for (const GardenTree& tree : lakeTrees)
        for (std::size_t cluster = 0; cluster < Tree::foliageOffsets.size(); ++cluster)
        {
            float r = Tree::foliageRadii[cluster] * tree.scale;
            appendPlantEllipsoid(vertices, indices, tree.root + Tree::foliageOffsets[cluster] * tree.scale,
                                 {r, r, r});
        }
    finishRange(clusteredFoliage);

    columnarFoliage.firstIndex = indices.size();
    for (const GardenTree& tree : trees)
    {
        if (tree.kind != 1) continue;
        appendPlantEllipsoid(vertices, indices,
            tree.root + Vec3{0, 4.8f, 0} * tree.scale,
            Vec3{1.45f, 2.4f, 1.45f} * tree.scale);
    }
    finishRange(columnarFoliage);

    coniferFoliage.firstIndex = indices.size();
    for (const GardenTree& tree : trees)
    {
        if (tree.kind != 2) continue;
        for (int tier = 0; tier < 3; ++tier)
        {
            appendConiferTier(vertices, indices,
                tree.root + Vec3{0, 2.0f + 1.4f * tier, 0} * tree.scale,
                (1.55f - 0.34f * tier) * tree.scale,
                (3.2f - 0.4f * tier) * tree.scale);
        }
    }
    finishRange(coniferFoliage);

    struct FlowerPlant
    {
        Vec3 position;
        int color;
        float groundHeight = 0.04f;
    };
    std::vector<FlowerPlant> flowers;
    const float flowerRadii[] = {18.75f, 23.5f, 21.25f, 28.0f};
    for (int sector = 0; sector < 8; sector += 2)
    {
        for (int color = 0; color < 4; ++color)
        {
            for (float fraction : {0.35f, 0.5f, 0.65f})
            {
                Vec3 groupCenter = sectorPlantPosition(sector, flowerRadii[color], fraction);
                for (Vec3 offset : {Vec3{0, 0, 0}, Vec3{0.28f, 0, 0},
                                    Vec3{-0.28f, 0, 0}, Vec3{0, 0, 0.28f},
                                    Vec3{0, 0, -0.28f}})
                {
                    flowers.push_back({groupCenter + offset, color});
                }
            }
        }
    }

    auto addOuterFlowerGroup = [&](Vec3 groupCenter, int color)
    {
        if (!isOuterPlantingSite(groupCenter, 0.6f) ||
            !clearOfTreeCrowns(groupCenter, 0.6f)) return;
        for (int row = -1; row <= 1; ++row)
        {
            for (int column = -1; column <= 1; ++column)
            {
                flowers.push_back({groupCenter + Vec3{0.3f * column, 0, 0.3f * row},
                                   color, 0.018f});
            }
        }
    };
    for (int group = 0; group < 3; ++group)
    {
        for (float side : {-1.0f, 1.0f})
        {
            addOuterFlowerGroup({-74.0f - 12.0f * group, 0.018f, side * 10.5f}, group);
        }
        addOuterFlowerGroup({-78.0f + 24.0f * group, 0.018f, -100.5f}, (group + 1) % 4);
    }

    for (std::size_t i = 0; i < lakeFlowers.size(); ++i)
        flowers.push_back({lakeFlowers[i], static_cast<int>((i / 5) % 4), 0.018f});

    hedgesAndLeaves.firstIndex = indices.size();
    // 160 clipped shrubs trace all eight planting sectors, inside the middle ring.
    for (int sector = 0; sector < 8; ++sector)
    {
        for (int shrub = 0; shrub < 20; ++shrub)
        {
            Vec3 root = sectorPlantPosition(sector, 30.6f,
                0.2f + 0.6f * shrub / 19.0f);
            appendPlantEllipsoid(vertices, indices, root + Vec3{0, 0.32f, 0},
                                 {0.35f, 0.32f, 0.35f});
        }
    }
    auto addOuterShrub = [&](Vec3 root)
    {
        if (isOuterPlantingSite(root, 0.45f) && clearOfTreeCrowns(root, 0.45f))
        {
            appendPlantEllipsoid(vertices, indices, root + Vec3{0, 0.38f, 0},
                                 {0.45f, 0.38f, 0.45f});
        }
    };
    for (int shrub = 0; shrub < 25; ++shrub)
    {
        for (float side : {-1.0f, 1.0f})
        {
            addOuterShrub({-70.0f - shrub, 0.018f, side * 7.5f});
        }
    }
    for (int shrub = 0; shrub < 31; ++shrub)
    {
        addOuterShrub({-82.0f + 2.0f * shrub, 0.018f, -102.0f});
    }
    for (Vec3 root : lakeShrubs)
        appendPlantEllipsoid(vertices, indices, root + Vec3{0, 0.34f, 0}, {0.38f, 0.34f, 0.38f});
    for (Vec3 root : lakeGrasses)
        for (int blade = 0; blade < 7; ++blade)
        {
            float angle = blade * 2.0f * pi / 7.0f;
            Vec3 stemRoot = root + Vec3{0.035f * std::cos(angle), 0, 0.035f * std::sin(angle)};
            Vec3 tip = stemRoot + Vec3{0, 0.35f + 0.06f * (blade % 3), 0};
            appendSteelBeam(vertices, indices, stemRoot, tip, 0.012f);
            appendTropicalLeaf(vertices, indices, tip, angle, 0.20f, 0.022f);
        }
    for (const FlowerPlant& flower : flowers)
    {
        Vec3 p = flower.position;
        appendBox(vertices, indices, p.x - 0.018f, p.x + 0.018f,
                  flower.groundHeight, 0.44f, p.z - 0.018f, p.z + 0.018f);
        appendPlantEllipsoid(vertices, indices, {p.x, 0.19f, p.z},
                             {0.16f, 0.10f, 0.12f});
    }
    finishRange(hedgesAndLeaves);

    for (int color = 0; color < 4; ++color)
    {
        blossoms[color].firstIndex = indices.size();
        for (const FlowerPlant& flower : flowers)
        {
            if (flower.color == color)
            {
                appendPlantEllipsoid(vertices, indices,
                    {flower.position.x, 0.44f, flower.position.z},
                    {0.16f, 0.09f, 0.16f});
            }
        }
        finishRange(blossoms[color]);
    }

    // Southern gate: local (0, 90) -> world (-370, -220), facing world +Z.
    // All existing trees remain in the forecourt's four lawn openings.
    entrancePaving.firstIndex = indices.size();
    appendEntranceCourt(vertices, indices);
    finishRange(entrancePaving);

    entranceStone.firstIndex = indices.size();
    for (float x : {-38.0f, -29.0f, -15.0f, 15.0f, 29.0f, 38.0f})
    {
        appendBox(vertices, indices, x - 0.95f, x + 0.95f, 0.018f, 0.34f, 97.05f, 98.95f);
        appendBox(vertices, indices, x - 0.65f, x + 0.65f, 0.34f, 9.8f, 97.35f, 98.65f);
    }
    for (float x : {-38.0f, 38.0f})
    {
        appendBox(vertices, indices, x - 0.95f, x + 0.95f, 0.018f, 0.34f, 81.05f, 82.95f);
        appendBox(vertices, indices, x - 0.65f, x + 0.65f, 0.34f, 9.8f, 81.35f, 82.65f);
        appendBox(vertices, indices, x - 0.65f, x + 0.65f, 9.8f, 11.1f, 82, 98);
    }
    appendBox(vertices, indices, -40, 40, 9.8f, 11.7f, 97, 99);
    finishRange(entranceStone);

    entranceTrim.firstIndex = indices.size();
    appendBox(vertices, indices, -40.15f, 40.15f, 11.7f, 12.0f, 96.85f, 99.15f);
    for (float x : {-38.0f, -29.0f, -15.0f, 15.0f, 29.0f, 38.0f})
        appendBox(vertices, indices, x - 0.83f, x + 0.83f, 9.48f, 9.8f, 97.17f, 98.83f);
    for (float x : {-38.0f, 38.0f})
        appendBox(vertices, indices, x - 0.83f, x + 0.83f, 9.48f, 9.8f, 81.17f, 82.83f);
    for (int bed = 4; bed < 8; ++bed)
    {
        const PavingOpening& b = entranceOpenings[bed];
        appendBox(vertices, indices, b.minX, b.maxX, 0.018f, 0.18f, b.minZ, b.minZ + 0.12f);
        appendBox(vertices, indices, b.minX, b.maxX, 0.018f, 0.18f, b.maxZ - 0.12f, b.maxZ);
        appendBox(vertices, indices, b.minX, b.minX + 0.12f, 0.018f, 0.18f, b.minZ + 0.12f, b.maxZ - 0.12f);
        appendBox(vertices, indices, b.maxX - 0.12f, b.maxX, 0.018f, 0.18f, b.minZ + 0.12f, b.maxZ - 0.12f);
    }
    finishRange(entranceTrim);

    entranceSigns.firstIndex = indices.size();
    for (float side : {-1.0f, 1.0f})
        for (int language = 0; language < 3; ++language)
        {
            float x = side * (language - 1) * 28.0f;
            float halfSign = language == 1 ? 15.0f : 10.5f;
            float z = side > 0 ? 99.015f : 96.985f;
            std::size_t first = vertices.size();
            appendFace(vertices, indices,
                {x - side * halfSign, 10.0f, z}, {x + side * halfSign, 10.0f, z},
                {x - side * halfSign, 11.5f, z}, {x + side * halfSign, 11.5f, z}, {0, 0, side});
            for (int corner = 0; corner < 4; ++corner)
            {
                vertices[first + corner * 8 + 6] = corner % 2 == 0 ? 0.0f : 1.0f;
                vertices[first + corner * 8 + 7] = corner < 2
                    ? ((language + 1) * 128.0f - 0.5f) / 384.0f
                    : (language * 128.0f + 0.5f) / 384.0f;
            }
        }
    finishRange(entranceSigns);

    entranceSoil.firstIndex = indices.size();
    std::vector<Vec3> entranceFlowerPositions;
    for (int bed = 4; bed < 8; ++bed)
    {
        const PavingOpening& b = entranceOpenings[bed];
        appendRectangle(vertices, indices, b.minX + 0.12f, b.maxX - 0.12f,
                        b.minZ + 0.12f, b.maxZ - 0.12f, 0.12f);
        for (int row = 0; row < 2; ++row)
            for (int flower = 0; flower < 8; ++flower)
                entranceFlowerPositions.push_back({b.minX + 0.7f + flower * 0.8f,
                    0.12f, b.minZ + 0.55f + row * 0.9f});
    }
    finishRange(entranceSoil);
    entranceLeaves.firstIndex = indices.size();
    for (Vec3 p : entranceFlowerPositions)
    {
        appendBox(vertices, indices, p.x - 0.018f, p.x + 0.018f, p.y, 0.44f, p.z - 0.018f, p.z + 0.018f);
        appendPlantEllipsoid(vertices, indices, {p.x, 0.26f, p.z}, {0.17f, 0.07f, 0.13f});
    }
    finishRange(entranceLeaves);
    for (int color = 0; color < 2; ++color)
    {
        entranceFlowers[color].firstIndex = indices.size();
        for (std::size_t i = 0; i < entranceFlowerPositions.size(); ++i)
            if (static_cast<int>((i / 8) % 2) == color)
            {
                Vec3 p = entranceFlowerPositions[i];
                appendPlantEllipsoid(vertices, indices, {p.x, 0.44f, p.z}, {0.15f, 0.09f, 0.15f});
            }
        finishRange(entranceFlowers[color]);
    }

    greenhouseStone.firstIndex = indices.size();
    appendBox(vertices, indices, greenhouseMinX, greenhouseMaxX, 0.018f, 0.09f,
              greenhouseBack, greenhouseFront);
    for (float x : {greenhouseMinX, greenhouseMaxX})
    {
        appendBox(vertices, indices, x - 0.09f, x + 0.09f, 0.09f, 0.36f,
                  greenhouseBack, greenhouseFront);
    }
    appendBox(vertices, indices, 30, 56, 0.09f, 0.36f, -82.09f, -81.91f);
    appendBox(vertices, indices, 30, 41.8f, 0.09f, 0.36f, -64.09f, -63.91f);
    appendBox(vertices, indices, 44.2f, 56, 0.09f, 0.36f, -64.09f, -63.91f);
    appendBox(vertices, indices, 40, 46, 0.018f, 0.09f, -64, -61);
    // A shallow connecting ramp meets the existing diagonal at (43, -43).
    appendQuad(vertices, indices, {41.25f, 0.09f, -61}, {44.75f, 0.09f, -61},
               {41.25f, 0.052f, -43}, {44.75f, 0.052f, -43});
    for (float x : {41.25f, 44.75f})
    {
        appendFace(vertices, indices, {x, 0.018f, -61}, {x, 0.09f, -61},
                   {x, 0.018f, -43}, {x, 0.052f, -43},
                   {x < 43.0f ? -1.0f : 1.0f, 0, 0});
    }
    for (float x : {35.0f, 51.0f})
    {
        for (float z : {-69.0f, -74.0f, -79.0f})
        {
            appendBox(vertices, indices, x - 1.1f, x + 1.1f, 0.09f, 0.55f,
                      z - 0.8f, z + 0.8f);
        }
    }
    const std::vector<ConservatoryPlant> interiorPlants = conservatoryPlants();
    for (const ConservatoryPlant& plant : interiorPlants)
    {
        if (!plant.palm)
            appendBox(vertices, indices, plant.root.x - 0.75f, plant.root.x + 0.75f,
                      0.09f, 0.48f, plant.root.z - 0.80f, plant.root.z + 0.80f);
    }
    finishRange(greenhouseStone);

    greenhouseSoil.firstIndex = indices.size();
    for (float x : {35.0f, 51.0f})
        for (float z : {-69.0f, -74.0f, -79.0f})
            appendQuad(vertices, indices, {x - 0.99f, 0.556f, z - 0.69f},
                {x + 0.99f, 0.556f, z - 0.69f}, {x - 0.99f, 0.556f, z + 0.69f},
                {x + 0.99f, 0.556f, z + 0.69f});
    for (const ConservatoryPlant& plant : interiorPlants)
    {
        if (!plant.palm)
            appendQuad(vertices, indices,
                {plant.root.x - 0.64f, 0.486f, plant.root.z - 0.69f},
                {plant.root.x + 0.64f, 0.486f, plant.root.z - 0.69f},
                {plant.root.x - 0.64f, 0.486f, plant.root.z + 0.69f},
                {plant.root.x + 0.64f, 0.486f, plant.root.z + 0.69f});
    }
    finishRange(greenhouseSoil);

    greenhouseFrame.firstIndex = indices.size();
    for (const GreenhouseHall& hall : greenhouseHalls)
    {
        float middle = (hall.left + hall.right) * 0.5f;
        for (int rib = 0; rib <= 6; ++rib)
        {
            float z = greenhouseBack + 3.0f * rib;
            appendSteelBeam(vertices, indices, {hall.left, greenhouseEaves, z},
                            {middle, hall.peak, z}, 0.10f);
            appendSteelBeam(vertices, indices, {middle, hall.peak, z},
                            {hall.right, greenhouseEaves, z}, 0.10f);
            // Shared valley columns are emitted once.
            appendSteelBeam(vertices, indices, {hall.left, 0.36f, z},
                            {hall.left, greenhouseEaves, z}, 0.09f);
            if (hall.right == greenhouseMaxX)
                appendSteelBeam(vertices, indices, {hall.right, 0.36f, z},
                                {hall.right, greenhouseEaves, z}, 0.09f);
        }
        for (int line = 0; line <= 6; ++line)
        {
            if (line == 6 && hall.right != greenhouseMaxX) continue;
            float x = hall.left + (hall.right - hall.left) * line / 6.0f;
            float y = hallRoofHeight(hall, x);
            appendSteelBeam(vertices, indices, {x, y, greenhouseBack},
                            {x, y, greenhouseFront}, 0.065f);
        }
        for (float z : {greenhouseBack, greenhouseFront})
        {
            for (int column = 0; column <= 4; ++column)
            {
                if (column == 4 && hall.right != greenhouseMaxX) continue;
                float x = hall.left + (hall.right - hall.left) * column / 4.0f;
                float bottom = z == greenhouseFront && x >= 40 && x <= 46
                    ? vestibuleRoofHeight(x) : 0.36f;
                appendSteelBeam(vertices, indices, {x, bottom, z},
                                {x, hallRoofHeight(hall, x), z}, 0.075f);
            }
        }
    }
    // Small triangular vestibule occupies the approved 6 x 3 entrance landing.
    for (float z : {-64.0f, -62.5f, -61.0f})
    {
        appendSteelBeam(vertices, indices, {40, 1.2f, z}, {43, 4.8f, z}, 0.085f);
        appendSteelBeam(vertices, indices, {43, 4.8f, z}, {46, 1.2f, z}, 0.085f);
        for (float x : {40.0f, 46.0f})
            appendSteelBeam(vertices, indices, {x, 0.09f, z}, {x, 1.2f, z}, 0.075f);
    }
    for (float x : {40.0f, 41.5f, 43.0f, 44.5f, 46.0f})
        appendSteelBeam(vertices, indices, {x, vestibuleRoofHeight(x), -64},
                        {x, vestibuleRoofHeight(x), -61}, 0.06f);
    for (float x : {41.8f, 43.0f, 44.2f})
        appendSteelBeam(vertices, indices, {x, 0.09f, -60.97f},
                        {x, 2.5f, -60.97f}, 0.075f);
    appendSteelBeam(vertices, indices, {41.8f, 2.5f, -60.97f},
                    {44.2f, 2.5f, -60.97f}, 0.085f);
    for (float x : {42.85f, 43.15f})
        appendSteelBeam(vertices, indices, {x, 1.05f, -60.88f},
                        {x, 1.5f, -60.88f}, 0.04f);
    // Small hinge plates and door bottom rails, without widening the doorway.
    for (float x : {41.8f, 44.2f})
        for (float y : {0.45f, 2.12f})
            appendBox(vertices, indices, x - 0.055f, x + 0.055f,
                      y - 0.09f, y + 0.09f, -61.025f, -60.915f);
    appendSteelBeam(vertices, indices, {41.8f, 0.16f, -60.97f},
                    {44.2f, 0.16f, -60.97f}, 0.055f);
    // Compact ridge connectors at the existing roof ribs, not new roof volumes.
    for (const GreenhouseHall& hall : greenhouseHalls)
        for (int rib = 0; rib <= 6; ++rib)
        {
            float x = (hall.left + hall.right) * 0.5f;
            float z = greenhouseBack + 3.0f * rib;
            appendBox(vertices, indices, x - 0.07f, x + 0.07f,
                      hall.peak - 0.13f, hall.peak + 0.025f, z - 0.085f, z + 0.085f);
        }
    finishRange(greenhouseFrame);

    greenhouseStems.firstIndex = indices.size();
    for (const ConservatoryPlant& plant : interiorPlants)
    {
        Vec3 crown = plant.root + Vec3{0, plant.height, 0};
        appendSteelBeam(vertices, indices, plant.root, crown, plant.palm ? 0.09f : 0.045f);
    }
    finishRange(greenhouseStems);

    greenhousePlants.firstIndex = indices.size();
    for (float x : {35.0f, 51.0f})
    {
        for (float z : {-69.0f, -74.0f, -79.0f})
        {
            appendPlantEllipsoid(vertices, indices, {x, 1.7f, z}, {0.8f, 1.2f, 0.65f});
            appendPlantEllipsoid(vertices, indices, {x + 0.35f, 1.1f, z + 0.25f},
                                 {0.6f, 0.6f, 0.5f});
        }
    }
    finishRange(greenhousePlants);

    // All repeated leaves share one mesh and two restrained foliage colors.
    // Existing six shrubs remain intact in the darker foliage batch.
    for (int shade = 0; shade < 2; ++shade)
    {
        IndexRange& foliage = shade == 0 ? greenhousePlants : greenhouseFreshFoliage;
        if (shade == 1) foliage.firstIndex = indices.size();
        for (const ConservatoryPlant& plant : interiorPlants)
        {
            int leaves = plant.palm ? 7 : 9;
            for (int leaf = 0; leaf < leaves; ++leaf)
            {
                if (leaf % 2 != shade) continue;
                float angle = 2.0f * pi * leaf / leaves + plant.phase;
                Vec3 crown = plant.root + Vec3{0, plant.height, 0};
                appendTropicalLeaf(vertices, indices, crown, angle,
                    plant.palm ? 0.95f : 0.62f, plant.palm ? 0.13f : 0.16f);
            }
        }
        finishRange(foliage);
    }

    greenhouseGlass.firstIndex = indices.size();
    // Small panes support back-to-front ordering across the concave roof valleys.
    // No glass walls separate adjacent halls, or the vestibule from the interior.
    for (const GreenhouseHall& hall : greenhouseHalls)
    {
        for (int strip = 0; strip < 6; ++strip)
        {
            float x0 = hall.left + (hall.right - hall.left) * strip / 6.0f;
            float x1 = hall.left + (hall.right - hall.left) * (strip + 1) / 6.0f;
            float y0 = hallRoofHeight(hall, x0), y1 = hallRoofHeight(hall, x1);
            Vec3 normal = normalize(Vec3{y0 - y1, x1 - x0, 0});
            for (int bay = 0; bay < 6; ++bay)
            {
                float z0 = greenhouseBack + bay * 3.0f, z1 = z0 + 3.0f;
                appendFace(vertices, indices, {x0, y0, z0}, {x1, y1, z0},
                           {x0, y0, z1}, {x1, y1, z1}, normal);
            }
        }
        // Split the front at every vestibule roof break, keeping its opening clear.
        std::vector<float> divisions{hall.left, (hall.left + hall.right) * 0.5f, hall.right};
        if (hall.left == 38.0f) divisions = {38, 40, 43, 46, 48};
        for (float z : {greenhouseBack, greenhouseFront})
        {
            for (std::size_t panel = 0; panel + 1 < divisions.size(); ++panel)
            {
                float x0 = divisions[panel], x1 = divisions[panel + 1];
                bool entranceOpening = z == greenhouseFront && x0 >= 40 && x1 <= 46;
                float low0 = entranceOpening ? vestibuleRoofHeight(x0) : 0.36f;
                float low1 = entranceOpening ? vestibuleRoofHeight(x1) : 0.36f;
                appendFace(vertices, indices, {x0, low0, z}, {x1, low1, z},
                           {x0, hallRoofHeight(hall, x0), z},
                           {x1, hallRoofHeight(hall, x1), z},
                           {0, 0, z == greenhouseFront ? 1.0f : -1.0f});
            }
        }
    }
    for (float x : {greenhouseMinX, greenhouseMaxX})
    {
        for (int bay = 0; bay < 6; ++bay)
        {
            float z0 = greenhouseBack + 3.0f * bay, z1 = z0 + 3.0f;
            appendFace(vertices, indices, {x, 0.36f, z0}, {x, greenhouseEaves, z0},
                       {x, 0.36f, z1}, {x, greenhouseEaves, z1},
                       {x == greenhouseMinX ? -1.0f : 1.0f, 0, 0});
        }
    }
    for (int strip = 0; strip < 4; ++strip)
    {
        float x0 = 40.0f + 1.5f * strip, x1 = x0 + 1.5f;
        float y0 = vestibuleRoofHeight(x0), y1 = vestibuleRoofHeight(x1);
        appendFace(vertices, indices, {x0, y0, -64}, {x1, y1, -64},
                   {x0, y0, -61}, {x1, y1, -61}, normalize(Vec3{y0 - y1, x1 - x0, 0}));
        appendFace(vertices, indices, {x0, 0.09f, -61}, {x1, 0.09f, -61},
                   {x0, y0, -61}, {x1, y1, -61}, {0, 0, 1});
    }
    for (float x : {40.0f, 46.0f})
        appendFace(vertices, indices, {x, 0.09f, -64}, {x, 1.2f, -64},
                   {x, 0.09f, -61}, {x, 1.2f, -61}, {x < 43 ? -1.0f : 1.0f, 0, 0});
    finishRange(greenhouseGlass);

    for (std::size_t i = greenhouseGlass.firstIndex; i < indices.size(); i += 3)
    {
        Vec3 midpoint{0, 0, 0};
        for (int corner = 0; corner < 3; ++corner)
        {
            std::size_t vertex = static_cast<std::size_t>(indices[i + corner]) * 8;
            midpoint = midpoint + Vec3{vertices[vertex], vertices[vertex + 1], vertices[vertex + 2]};
        }
        midpoint = midpoint * (1.0f / 3.0f);
        Vec3 world{
            model[0] * midpoint.x + model[4] * midpoint.y + model[8] * midpoint.z + model[12],
            model[1] * midpoint.x + model[5] * midpoint.y + model[9] * midpoint.z + model[13],
            model[2] * midpoint.x + model[6] * midpoint.y + model[10] * midpoint.z + model[14]
        };
        glassTriangles.push_back({{indices[i], indices[i + 1], indices[i + 2]}, world});
    }
    glGenBuffers(1, &glassEbo);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                 indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          reinterpret_cast<const void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    entranceSignTexture = createEntranceSignTexture();
}

BotanicalGarden::~BotanicalGarden()
{
    glDeleteTextures(1, &entranceSignTexture);
    glDeleteBuffers(1, &glassEbo);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void BotanicalGarden::drawRange(const IndexRange& range) const
{
    glDrawElements(GL_TRIANGLES, range.count, GL_UNSIGNED_INT,
                   reinterpret_cast<const void*>(range.firstIndex * sizeof(GLuint)));
}

void BotanicalGarden::render(GLint modelLocation, GLint objectColorLocation,
                             GLint useObjectColorLocation, GLint useGrassLocation,
                             GLint useTextureLocation, GLint textureRepeatLocation,
                             GLint botanicalLakeLocation, GLuint stoneTexture) const
{
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
    glUniform1i(useObjectColorLocation, GL_TRUE);
    glUniform1i(useTextureLocation, GL_FALSE);
    glBindVertexArray(vao);

    glUniform1i(useGrassLocation, GL_TRUE);
    glUniform3f(objectColorLocation, 0.37f, 0.53f, 0.35f);
    drawRange(district);
    glUniform3f(objectColorLocation, 0.40f, 0.56f, 0.37f);
    drawRange(lightSectors);
    glUniform3f(objectColorLocation, 0.35f, 0.51f, 0.34f);
    drawRange(darkSectors);
    glUniform3f(objectColorLocation, 0.34f, 0.48f, 0.31f);
    drawRange(lakeGrassBank);
    glUniform1i(useGrassLocation, GL_FALSE);

    glUniform3f(objectColorLocation, 0.42f, 0.37f, 0.29f);
    drawRange(lakeSoilBank);
    glUniform3f(objectColorLocation, 0.35f, 0.30f, 0.25f);
    drawRange(flowerSoil);
    glUniform3f(objectColorLocation, 0.59f, 0.20f, 0.24f);
    drawRange(flowerRed);
    glUniform3f(objectColorLocation, 0.79f, 0.61f, 0.25f);
    drawRange(flowerGold);
    glUniform3f(objectColorLocation, 0.46f, 0.34f, 0.57f);
    drawRange(flowerPurple);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, stoneTexture);
    glUniform2f(textureRepeatLocation, 1.0f, 1.0f);
    glUniform1i(useTextureLocation, GL_TRUE);
    glUniform3f(objectColorLocation, 0.78f, 0.79f, 0.76f);
    drawRange(radialPaths);
    drawRange(outwardPaths);
    drawRange(mainAxis);
    drawRange(lakeShorePath);
    glUniform3f(objectColorLocation, 0.84f, 0.84f, 0.81f);
    drawRange(innerCurvedPath);
    drawRange(middleCurvedPath);
    drawRange(ornamentalPaths);
    drawRange(circularPath);
    drawRange(centralPlaza);
    drawRange(bridgeDecks);
    drawRange(greenhouseStone);
    drawRange(entrancePaving);

    glUniform1i(useTextureLocation, GL_FALSE);
    glUniform1i(botanicalLakeLocation, GL_TRUE);
    glUniform3f(objectColorLocation, 0.26f, 0.52f, 0.55f);
    drawRange(lakeWater);
    glUniform1i(botanicalLakeLocation, GL_FALSE);

    glUniform3f(objectColorLocation, 0.78f, 0.81f, 0.81f);
    drawRange(bridgeRails);
    glUniform3f(objectColorLocation, 0.20f, 0.23f, 0.25f);
    drawRange(lakesideBenchFrames);
    glUniform3f(objectColorLocation, 0.43f, 0.29f, 0.18f);
    drawRange(lakesideWood);
    glUniform3f(objectColorLocation, 0.86f, 0.88f, 0.84f);
    drawRange(lakesideDiffusers);
    glUniform3f(objectColorLocation, 0.70f, 0.72f, 0.69f);
    drawRange(border);
    glUniform3f(objectColorLocation, 0.72f, 0.77f, 0.78f);
    drawRange(greenhouseFrame);
    glUniform3f(objectColorLocation, 0.20f, 0.40f, 0.24f);
    drawRange(greenhousePlants);
    glUniform3f(objectColorLocation, 0.29f, 0.47f, 0.25f);
    drawRange(greenhouseFreshFoliage);
    glUniform3f(objectColorLocation, 0.34f, 0.30f, 0.19f);
    drawRange(greenhouseStems);
    glUniform3f(objectColorLocation, 0.24f, 0.20f, 0.15f);
    drawRange(greenhouseSoil);
    glUniform3f(objectColorLocation, 0.88f, 0.87f, 0.83f);
    drawRange(entranceStone);
    glUniform3f(objectColorLocation, 0.78f, 0.78f, 0.74f);
    drawRange(entranceTrim);
    glUniform3f(objectColorLocation, 0.30f, 0.25f, 0.18f);
    drawRange(entranceSoil);
    glUniform3f(objectColorLocation, 0.26f, 0.43f, 0.23f);
    drawRange(entranceLeaves);
    glUniform3f(objectColorLocation, 0.70f, 0.20f, 0.23f);
    drawRange(entranceFlowers[0]);
    glUniform3f(objectColorLocation, 0.91f, 0.90f, 0.83f);
    drawRange(entranceFlowers[1]);

    // Static vegetation shares this VAO and uses nine material batches.
    glUniform3f(objectColorLocation, 0.36f, 0.22f, 0.12f);
    drawRange(treeTrunks);
    glUniform3f(objectColorLocation, 0.22f, 0.44f, 0.23f);
    drawRange(clusteredFoliage);
    glUniform3f(objectColorLocation, 0.27f, 0.46f, 0.24f);
    drawRange(columnarFoliage);
    glUniform3f(objectColorLocation, 0.17f, 0.34f, 0.25f);
    drawRange(coniferFoliage);
    glUniform3f(objectColorLocation, 0.25f, 0.42f, 0.22f);
    drawRange(hedgesAndLeaves);
    const Vec3 blossomColors[] = {
        {0.65f, 0.19f, 0.23f}, {0.85f, 0.70f, 0.27f},
        {0.88f, 0.87f, 0.80f}, {0.50f, 0.33f, 0.61f}
    };
    for (int color = 0; color < 4; ++color)
    {
        Vec3 value = blossomColors[color];
        glUniform3f(objectColorLocation, value.x, value.y, value.z);
        drawRange(blossoms[color]);
    }
    glUniform1i(useTextureLocation, GL_TRUE);
    glUniform2f(textureRepeatLocation, 1.0f, 1.0f);
    glUniform3f(objectColorLocation, 1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, entranceSignTexture);
    drawRange(entranceSigns);
    glUniform1i(useTextureLocation, GL_FALSE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

void BotanicalGarden::renderGreenhouseGlass(GLint modelLocation,
                                            GLint objectColorLocation,
                                            GLint glassLocation, const Mat4& view) const
{
    struct OrderedTriangle { float depth; std::size_t triangle; };
    std::vector<OrderedTriangle> order;
    order.reserve(glassTriangles.size());
    for (std::size_t i = 0; i < glassTriangles.size(); ++i)
    {
        Vec3 p = glassTriangles[i].worldCenter;
        float depth = view[2] * p.x + view[6] * p.y + view[10] * p.z + view[14];
        order.push_back({depth, i});
    }
    std::stable_sort(order.begin(), order.end(),
        [](const OrderedTriangle& a, const OrderedTriangle& b) { return a.depth < b.depth; });
    std::vector<GLuint> sortedIndices;
    sortedIndices.reserve(order.size() * 3);
    for (const OrderedTriangle& entry : order)
    {
        const auto& triangle = glassTriangles[entry.triangle].indices;
        sortedIndices.insert(sortedIndices.end(), triangle.begin(), triangle.end());
    }
    GLboolean oldDepthWrite;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthWrite);
    GLboolean oldBlend = glIsEnabled(GL_BLEND);
    GLboolean oldCull = glIsEnabled(GL_CULL_FACE);
    GLboolean oldDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLint oldCullMode, sourceRgb, destinationRgb, sourceAlpha, destinationAlpha;
    GLint equationRgb, equationAlpha;
    glGetIntegerv(GL_CULL_FACE_MODE, &oldCullMode);
    glGetIntegerv(GL_BLEND_SRC_RGB, &sourceRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &destinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &sourceAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &destinationAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &equationRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &equationAlpha);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
    glUniform3f(objectColorLocation, 0.46f, 0.69f, 0.68f);
    glUniform1i(glassLocation, GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Each glass pane is drawn once, visible from either side. The shader
    // orients its normal toward the viewer; sorting replaces the convex-shell pass.
    glDisable(GL_CULL_FACE);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glassEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sortedIndices.size() * sizeof(GLuint),
                 sortedIndices.data(), GL_STREAM_DRAW);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sortedIndices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindVertexArray(0);
    glUniform1i(glassLocation, GL_FALSE);

    glDepthMask(oldDepthWrite);
    glCullFace(oldCullMode);
    glBlendFuncSeparate(sourceRgb, destinationRgb, sourceAlpha, destinationAlpha);
    glBlendEquationSeparate(equationRgb, equationAlpha);
    if (!oldBlend) glDisable(GL_BLEND);
    if (oldCull) glEnable(GL_CULL_FACE);
    if (!oldDepthTest) glDisable(GL_DEPTH_TEST);
}
