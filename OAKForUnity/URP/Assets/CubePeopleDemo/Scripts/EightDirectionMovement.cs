using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace CubePeople
{
    public class EightDirectionMovement : MonoBehaviour
    {

        public float velocity = 5;
        public float turnSpeed = 10;

        Vector2 input;
        float angle;

        Quaternion targetRotation;
        public Transform cam; //Transform cam;

        FollowTarget ft;

        void Start()
        {
            //cam = Camera.main.transform;
            if (cam.GetComponent<FollowTarget>())
            {
                ft = cam.GetComponent<FollowTarget>();
            }

        }

        void Update()
        {
            GetInput();

            if (Mathf.Abs(input.x) < 1 && Mathf.Abs(input.y) < 1) return;

            CalculateDirection();
            Rotate();
            Move();

        }

        void GetInput()
        {
            input.x = Input.GetAxisRaw("Horizontal");
            input.y = Input.GetAxisRaw("Vertical");
        }

        void CalculateDirection()
        {
            angle = Mathf.Atan2(input.x, input.y);
            angle = Mathf.Rad2Deg * angle;
            angle += cam.eulerAngles.y;
        }

        void Rotate()
        {
            if (ft != null && ft.camRotation)
            {
                transform.rotation = Quaternion.Euler(0, input.x * 1.5f, 0) * transform.rotation;
            }
            else
            {
                targetRotation = Quaternion.Euler(0, angle, 0);
            }

            transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, turnSpeed * Time.deltaTime);
        }

        void Move()
        {
            transform.position += transform.forward * velocity * Time.deltaTime;
        }
    }
}
