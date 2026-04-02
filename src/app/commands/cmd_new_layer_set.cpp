// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/status_bar.h"
#include "app/transaction.h"
#include "app/ui/main_window.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <cstdio>
#include <cstring>

namespace app {

static std::string get_unique_layer_set_name(Sprite* sprite);
static int get_max_layer_set_num(Layer* layer);
static Layer* get_top_visible_layer(Layer* layer);

class NewLayerSetCommand : public Command {
public:
  NewLayerSetCommand();
  Command* clone() const override { return new NewLayerSetCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

NewLayerSetCommand::NewLayerSetCommand()
  : Command("NewLayerSet",
            "New Layer Set",
            CmdRecordableFlag)
{
}

bool NewLayerSetCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewLayerSetCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  Layer* activeLayer = writer.layer();
  Layer* topLayer = get_top_visible_layer(activeLayer);
  std::string name = get_unique_layer_set_name(sprite);
  Layer* layer;
  {
    Transaction transaction(writer.context(), "New Layer");
    DocumentApi api = document->getApi(transaction);
    layer = api.newLayerFolder(sprite);

    // Create the new folder above the active layer or active layer set.
    if (topLayer)
      api.restackLayerAfter(layer, topLayer);

    layer->setName(name);
    transaction.commit();
  }

  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  StatusBar::instance()->showTip(1000, "Layer `%s' created", name.c_str());

  App::instance()->mainWindow()->popTimeline();
}

static std::string get_unique_layer_set_name(Sprite* sprite)
{
  char buf[1024];
  std::snprintf(buf, sizeof(buf), "Layer Set %d", get_max_layer_set_num(sprite->folder())+1);
  return buf;
}

static int get_max_layer_set_num(Layer* layer)
{
  int max = 0;

  if (std::strncmp(layer->name().c_str(), "Layer Set ", 10) == 0)
    max = std::strtol(layer->name().c_str()+10, NULL, 10);

  if (layer->isFolder()) {
    LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
    LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

    for (; it != end; ++it) {
      int tmp = get_max_layer_set_num(*it);
      max = MAX(tmp, max);
    }
  }

  return max;
}

static Layer* get_top_visible_layer(Layer* layer)
{
  if (!layer)
    return nullptr;

  while (layer->parent() && layer->parent()->parent())
    layer = layer->parent();

  return layer;
}

Command* CommandFactory::createNewLayerSetCommand()
{
  return new NewLayerSetCommand;
}

} // namespace app
