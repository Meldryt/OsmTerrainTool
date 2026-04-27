#include "RhiTextureItem.h"
#include <QFile>
#include "ElevationStore.h"

QQuickRhiItemRenderer *RhiItem::createRenderer()
{
    return new RhiItemRenderer;
}

void RhiItem::setBackgroundAlpha(float a)
{
    if (m_alpha == a)
        return;

    m_alpha = a;
    emit backgroundAlphaChanged();
    update();
}

void RhiItem::updateBuffers()
{
    ++m_bufferUpdateCount;
    emit dynamicBufferUpdateChanged();
    update();
}

void RhiItemRenderer::synchronize(QQuickRhiItem *rhiItem)
{
    RhiItem *item = static_cast<RhiItem *>(rhiItem);
    if (item->backgroundAlpha() != m_alpha)
    {
        m_alpha = item->backgroundAlpha();
    }

    if (item->bufferUpdateCount() != m_bufferUpdateCount)
    {
        m_bufferUpdateCount = item->bufferUpdateCount();
    }
}

static QShader getShader(const QString &name)
{
    QFile f(name);
    return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
}

void RhiItemRenderer::initialize(QRhiCommandBuffer *cb)
{
    if (m_rhi != rhi()) {
        m_rhi = rhi();
        m_pipeline.reset();
    }

    if (m_sampleCount != renderTarget()->sampleCount()) {
        m_sampleCount = renderTarget()->sampleCount();
        m_pipeline.reset();
    }


    QRhiTexture *finalTex = m_sampleCount > 1 ? resolveTexture() : colorTexture();
    if (m_textureFormat != finalTex->format()) {
        m_textureFormat = finalTex->format();
        m_pipeline.reset();
    }

    if (!m_pipeline) {
        //m_lastHeightCount = ElevationStore::instance()->heightCount();

        createVertexBuffers(cb);

        m_ubufStatic.reset(m_rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::UniformBuffer, 64));
        if(!m_ubufStatic->create())
        {
            qCritical() << __FUNCTION__ << " could not create buffer";
        }

        m_ubufDynamic.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
        if(!m_ubufDynamic->create())
        {
            qCritical() << __FUNCTION__ << " could not create buffer";
        }

        m_srb.reset(m_rhi->newShaderResourceBindings());
        m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, m_ubufStatic.get()),
            QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::VertexStage, m_ubufDynamic.get()),
        });
        if(!m_srb->create())
        {
            qCritical() << __FUNCTION__ << " could not create buffer";
        }

        m_pipeline.reset(m_rhi->newGraphicsPipeline());
        m_pipeline->setTopology(QRhiGraphicsPipeline::Topology::TriangleStrip);

        m_pipeline->setShaderStages({
           { QRhiShaderStage::Vertex, getShader(QLatin1String(":/osmterraintool/resources/shaders/color.vert.qsb")) },
           { QRhiShaderStage::Fragment, getShader(QLatin1String(":/osmterraintool/resources/shaders/color.frag.qsb")) }
        });
        m_pipeline->setSampleCount(m_sampleCount);

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 2 * sizeof(float) }, //float x, float y
            { 1 * sizeof(int32_t) } //int32_t height
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
            { 1, 1, QRhiVertexInputAttribute::SInt, 0 }
        });

        m_pipeline->setVertexInputLayout(inputLayout);
        m_pipeline->setShaderResourceBindings(m_srb.get());
        m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
        m_pipeline->create();

        m_viewProjection.ortho(-1.0f,1.0f,-1.0f,1.0f, -1.0f, 1.0f);

        const QMatrix4x4 modelViewProjection = m_viewProjection;
        const int32_t minHeight = 0;
        const int32_t maxHeight = 5000;

        QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();
        resourceUpdates->uploadStaticBuffer(m_ubufStatic.get(), 0, 64, modelViewProjection.constData());
        resourceUpdates->updateDynamicBuffer(m_ubufDynamic.get(), 0, 32, &minHeight);
        resourceUpdates->updateDynamicBuffer(m_ubufDynamic.get(), 32, 32, &maxHeight);
        cb->resourceUpdate(resourceUpdates);
    }
}

void RhiItemRenderer::createVertexBuffers(QRhiCommandBuffer *cb)
{
    createVertexData();
    createIndexData();

    m_vbufPos.reset(m_rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::VertexBuffer, sizeof(m_vertexPosData[0])*m_vertexPosData.size()));
    if(!m_vbufPos->create())
    {
        qCritical() << __FUNCTION__ << " could not create buffer";
    }

    m_vbufHeight.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, sizeof(m_vertexHeightData[0])*m_vertexHeightData.size()));
    if(!m_vbufHeight->create())
    {
        qCritical() << __FUNCTION__ << " could not create buffer";
    }

    m_ibuf.reset(m_rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::IndexBuffer, sizeof(m_indexData[0])*m_indexData.size()));
    if(!m_ibuf->create())
    {
        qCritical() << __FUNCTION__ << " could not create buffer";
    }

    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();
    resourceUpdates->uploadStaticBuffer(m_vbufPos.get(), 0, sizeof(m_vertexPosData[0])*m_vertexPosData.size(), &m_vertexPosData[0]);
    resourceUpdates->uploadStaticBuffer(m_ibuf.get(), 0, sizeof(m_indexData[0])*m_indexData.size(), &m_indexData[0]);
    cb->resourceUpdate(resourceUpdates);
}

void RhiItemRenderer::createVertexData()
{
    m_vertexPosData.clear();
    m_vertexHeightData.clear();

    const uint32_t heightCount = ElevationStore::instance()->heightCount();

    for(uint32_t y=0;y<heightCount;++y)
    {
        for(uint32_t x=0;x<heightCount;++x)
        {
            const float xPos = -1.0f + float(x*2.0f)/heightCount;
            const float yPos = -1.0f + float(y*2.0f)/heightCount;
            m_vertexPosData.push_back(xPos);
            m_vertexPosData.push_back(yPos);
            m_vertexHeightData.push_back(-9999);
        }
    }
}

void RhiItemRenderer::createIndexData()
{
    m_indexData.clear();

    const uint32_t heightCount = ElevationStore::instance()->heightCount();
    const uint32_t max_x = heightCount;
    const uint32_t max_y = heightCount;

    //triangles
    // for(uint32_t y=0;y<max_y-1;++y)
    // {
    //     for(uint32_t x=0;x<max_x-1;++x)
    //     {
    //         const uint32_t i0 = x + (y*max_x);
    //         const uint32_t i1 = (x+1) + (y*max_x);
    //         const uint32_t i2 = x + ((y+1)*max_x);
    //         const uint32_t i3 = (x+1) + ((y+1)*max_x);
    //         //2-3
    //         //| |
    //         //0-1
    //         m_indexData.push_back(i0);
    //         m_indexData.push_back(i1);
    //         m_indexData.push_back(i2);
    //         m_indexData.push_back(i2);
    //         m_indexData.push_back(i3);
    //         m_indexData.push_back(i1);
    //     }
    //     m_indexData.push_back(0xFFFFFFFF);
    // }

    //m_indexData.clear();

    //triangle strip
    for(uint32_t y=0;y<max_y-1;++y)
    {
        for(uint32_t x=0;x<max_x;++x)
        {
            const uint32_t i1 = x + (y*max_x);
            const uint32_t i2 = x + ((y+1)*max_x);

            m_indexData.push_back(i1);
            m_indexData.push_back(i2);
        }
        m_indexData.push_back(0xFFFFFFFF);
    }
}

void RhiItemRenderer::render(QRhiCommandBuffer *cb)
{
    static uint32_t lastBufferUpdateCount = 0;
    if(lastBufferUpdateCount != m_bufferUpdateCount)
    {
        updateBuffers(cb);
    }

    // Qt Quick expects premultiplied alpha
    const QColor clearColor = QColor::fromRgbF(0.0f,0.0,0.0f,0.0f);
    //const QColor clearColor = QColor::fromRgbF(1.0f * m_alpha, 0.0f * m_alpha, 0.0f * m_alpha, m_alpha);
    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();
    cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, resourceUpdates);
    cb->setGraphicsPipeline(m_pipeline.get());
    const QSize outputSize = renderTarget()->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { m_vbufPos.get(), 0 },
        { m_vbufHeight.get(), 0 }
    };
    cb->setVertexInput(0, 2, &vbufBindings[0], m_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(m_indexData.size());
    cb->endPass();

    lastBufferUpdateCount = m_bufferUpdateCount;
}

void RhiItemRenderer::updateBuffers(QRhiCommandBuffer *cb)
{
    ElevationStore* elevationStore = ElevationStore::instance();
    std::vector<int16_t>& heights = elevationStore->requestedHeights();
    if(heights.empty())
    {
        return;
    }

    static uint32_t lastHeightCount = 0;
    const uint32_t heightCount = elevationStore->heightCount();
    const int32_t minHeight = elevationStore->minHeight();
    const int32_t maxHeight = elevationStore->maxHeight();
    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();

    if(lastHeightCount != 0 && heightCount != lastHeightCount)
    {
        createVertexBuffers(cb);
    }

    for(uint32_t y=0;y<heightCount;++y)
    {
        for(uint32_t x=0;x<heightCount;++x)
        {
            const uint32_t index = x + y * heightCount;

            m_vertexHeightData[index] = heights[index];
        }
    }

    resourceUpdates->updateDynamicBuffer(m_vbufHeight.get(), 0, sizeof(m_vertexHeightData[0])*m_vertexHeightData.size(), &m_vertexHeightData[0]);
    resourceUpdates->updateDynamicBuffer(m_ubufDynamic.get(), 0, 32, &minHeight);
    resourceUpdates->updateDynamicBuffer(m_ubufDynamic.get(), 32, 32, &maxHeight);
    cb->resourceUpdate(resourceUpdates);

    lastHeightCount = heightCount;
}
