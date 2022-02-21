using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace CubePeople
{
    public class AnimationController : MonoBehaviour
    {

        Animator anim;
        public bool run;

        void Start()
        {
            anim = GetComponent<Animator>();
            if (run) run = false;
        }


        void Update()
        {

            if (Input.GetAxisRaw("Vertical") == 0 && Input.GetAxisRaw("Horizontal") == 0)
            {
                run = false;
            }
            else
            {
                run = true;
            }

            anim.SetBool("Run", run);
        }
    }
}
