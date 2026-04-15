#pragma once

#include <QQuickRhiItem>
#include <QGeoCoordinate>
#include <QGeoShape>
#include <rhi/qrhi.h>

class RhiItemRenderer : public QQuickRhiItemRenderer
{
public:
    void initialize(QRhiCommandBuffer *cb) override;
    void synchronize(QQuickRhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;
    void updateBuffers(QRhiCommandBuffer *cb);

private:
    void createVertexBuffers(QRhiCommandBuffer *cb);
    void createVertexData();
    void createIndexData();

    QRhi *m_rhi = nullptr;
    int m_sampleCount = 1;
    QRhiTexture::Format m_textureFormat = QRhiTexture::RGBA8;

    std::unique_ptr<QRhiBuffer> m_vbufPos;
    std::unique_ptr<QRhiBuffer> m_vbufHeight;
    std::unique_ptr<QRhiBuffer> m_ibuf;

    std::unique_ptr<QRhiBuffer> m_ubufStatic;
    std::unique_ptr<QRhiBuffer> m_ubufDynamic;

    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    QMatrix4x4 m_viewProjection;
    float m_alpha = 1.0f;
    uint32_t m_bufferUpdateCount = 0;
    //uint32_t m_lastHeightCount = 0;

    std::vector<float> m_vertexPosData;
    std::vector<int32_t> m_vertexHeightData;
    std::vector<uint32_t> m_indexData;
};

class RhiItem : public QQuickRhiItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RhiItem)
    Q_PROPERTY(float backgroundAlpha READ backgroundAlpha WRITE setBackgroundAlpha NOTIFY backgroundAlphaChanged)

public:
    QQuickRhiItemRenderer *createRenderer() override;

    float backgroundAlpha() const { return m_alpha; }
    void setBackgroundAlpha(float a);

    uint32_t bufferUpdateCount() const { return m_bufferUpdateCount; }
    Q_INVOKABLE void updateBuffers();

signals:
    void angleChanged();
    void backgroundAlphaChanged();
    void dynamicBufferUpdateChanged();

private:
    float m_alpha = 1.0f;
    std::vector<float> m_heights;
    uint32_t m_bufferUpdateCount{0};
};
