using UnityEngine;

/**
 * Animate color of cube when get collision with some magnitude threshold
 */
public class CubeController : MonoBehaviour
{
    // Use random color from list of colors
    public bool useRandomColor;
    public Color[] randomColors;
    public Color realColor;

    // prevent sequential animations
    private float _elapsedTime;
    private float _pauseTime = 0.0f;
    private bool _colorAnimate;
    private float _elapsedColorTime;
    private Renderer _cubeRenderer;
    private float _blinkTime = 3.0f;

    // Start is called before the first frame update
    void Start()
    {
        _colorAnimate = false;
        _cubeRenderer = GetComponent<Renderer>();

        if (useRandomColor) 
        {
            int rnd = Random.Range(0,4);
            realColor = randomColors[rnd];
        }
    }

    // Update is called once per frame
    void Update()
    {
        _elapsedTime += Time.deltaTime;
        
        // _colorAnimate only true if get previous collision
        if (_colorAnimate && _elapsedTime > _pauseTime)
        {
            _elapsedColorTime += Time.deltaTime;

            if (_elapsedColorTime < _blinkTime * 2 )
            {
                // Color lerp between white and random color
                Color lerpedColor = Color.Lerp(Color.white, realColor, Mathf.PingPong(Time.time,_blinkTime));
                _cubeRenderer.material.SetColor("_BaseColor", lerpedColor);
            }
            else 
            {
                // back to white color after some time
                _cubeRenderer.material.SetColor("_BaseColor", Color.white);
                _elapsedColorTime = 0.0f;
                _elapsedTime = 0.0f;
                _pauseTime = 5.0f;
                _colorAnimate = false;
            }
        }

    }

    void OnCollisionEnter(Collision collision)
    {
        if (collision.relativeVelocity.magnitude > 2)
        {
            _colorAnimate = true;
        }
    }
}
