#pragma once

#include "render_frame.hpp"
#include "render_camera.hpp"
#include "render_object.hpp"

#include <vector>

struct RenderScene
{
    RenderCamera camera;
    std::vector<RenderObjectPrototype> objects;
};

inline RenderFramePrototype BuildRenderFrameFromScene(const RenderScene& scene)
{
    RenderItemPrototype itemTemplate;
    itemTemplate.kind = RenderItemKind::Object3D;

    RenderViewPrototype view;
    view.kind = RenderViewKind::Scene3D;
    view.camera = scene.camera;
    view.items.reserve(scene.objects.size());

    for (const RenderObjectPrototype& object : scene.objects)
    {
        RenderItemPrototype item = itemTemplate;
        item.object3D = object;
        view.items.push_back(item);
    }

    RenderPassPrototype pass;
    pass.kind = RenderPassKind::Scene;
    pass.views.push_back(std::move(view));

    RenderFramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}
