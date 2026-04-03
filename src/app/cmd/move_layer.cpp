// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/move_layer.h"

#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

MoveLayer::MoveLayer(Layer* layer, Layer* afterThis)
  : m_layer(layer)
  , m_oldFolder(layer->parent())
  , m_newFolder(layer->parent())
  , m_oldAfterThis(layer->getPrevious())
  , m_newAfterThis(afterThis)
{
}

MoveLayer::MoveLayer(Layer* layer, LayerFolder* folder, Layer* afterThis)
  : m_layer(layer)
  , m_oldFolder(layer->parent())
  , m_newFolder(folder)
  , m_oldAfterThis(layer->getPrevious())
  , m_newAfterThis(afterThis)
{
}

void MoveLayer::onExecute()
{
  moveLayer(static_cast<LayerFolder*>(m_newFolder.layer()), m_newAfterThis.layer());
}

void MoveLayer::onUndo()
{
  moveLayer(static_cast<LayerFolder*>(m_oldFolder.layer()), m_oldAfterThis.layer());
}

void MoveLayer::onFireNotifications()
{
  Layer* layer = m_layer.layer();
  doc::Document* doc = layer->sprite()->document();
  DocumentEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onLayerRestacked, ev);
}

void MoveLayer::moveLayer(LayerFolder* newFolder, Layer* afterThis)
{
  ASSERT(newFolder != NULL);

  Layer* layer = m_layer.layer();
  LayerFolder* oldFolder = layer->parent();

  // Be defensive here: timeline drag/drop can provide a stale anchor when
  // moving layers across folder boundaries. In that case we just insert at
  // the beginning of the destination folder instead of crashing.
  if (afterThis && afterThis->parent() != newFolder)
    afterThis = nullptr;

  if (oldFolder != newFolder) {
    oldFolder->removeLayer(layer);
    newFolder->addLayer(layer);
    oldFolder->incrementVersion();
  }

  newFolder->stackLayer(layer, afterThis);
  newFolder->incrementVersion();
}

} // namespace cmd
} // namespace app
