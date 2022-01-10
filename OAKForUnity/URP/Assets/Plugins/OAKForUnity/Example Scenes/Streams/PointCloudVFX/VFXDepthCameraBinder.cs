/**
 * Textyre2D Binder based on Keijiro Takahashi DepthAITestbed repository
 * https://github.com/keijiro/DepthAITestbed
 */

using UnityEngine;
using UnityEngine.VFX;
using UnityEngine.VFX.Utility;

namespace OAKForUnity {

[AddComponentMenu("VFX/Property Binders/DepthCamera Binder")]
[VFXBinder("DepthCamera")]
class VFXDepthCameraBinder : VFXBinderBase
{
    [SerializeField] DaiPointCloudVFX _driver = null;

    public string MonoMapProperty
      { get => (string)_monoMapProperty;
        set => _monoMapProperty = value; }

    public string DepthMapProperty
      { get => (string)_depthMapProperty;
        set => _depthMapProperty = value; }

    [VFXPropertyBinding("UnityEngine.Texture2D"), SerializeField]
    ExposedProperty _monoMapProperty = "MonoMap";

    [VFXPropertyBinding("UnityEngine.Texture2D"), SerializeField]
    ExposedProperty _depthMapProperty = "DepthMap";

    public override bool IsValid(VisualEffect component)
      => _driver != null &&
         component.HasTexture(_monoMapProperty) &&
         component.HasTexture(_depthMapProperty);

    public override void UpdateBinding(VisualEffect component)
    {
        if (_driver.monoRTexture == null) return;

        component.SetTexture(_monoMapProperty, _driver.monoRTexture);
        component.SetTexture(_depthMapProperty, _driver.depthTexture);
    }

    public override string ToString()
      => $"DepthAI : {_monoMapProperty}, {_depthMapProperty}";
}

} // namespace OAKForUnity
