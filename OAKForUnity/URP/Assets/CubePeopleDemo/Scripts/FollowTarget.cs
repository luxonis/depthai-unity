using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace CubePeople
{
    public class FollowTarget : MonoBehaviour
    {

        public Transform target;
        public Vector3 offsetPos;
        public float moveSpeed = 5;
        public float turnSpeed = 10;
        public float smoothSpeed = 0.5f;

        public bool camRotation;    //TODO : If activated during play, an error remains.

        Quaternion targetRotation;
        Vector3 targetPos;
        bool smoothRotating = false;


        void Update()
        {

            if (!camRotation)
            {
                MoveWithTarget();
                LookAtTarget();
            }
            else
            {
                LookatRotation();
            }

            if (Input.GetKeyDown(KeyCode.G) && !smoothRotating)
            {
                StartCoroutine("RotateAroundTarget", 45);
            }

            if (Input.GetKeyDown(KeyCode.H) && !smoothRotating)
            {
                StartCoroutine("RotateAroundTarget", -45);
            }
        }
        /*
        void LateUpdate()
        {
            if (camRotation)
            {
                LookatRotation();
                print("LookatRotation");
            }
        }
        */
        void MoveWithTarget()
        {
            targetPos = target.transform.position + offsetPos;
            transform.position = Vector3.Lerp(transform.position, targetPos, moveSpeed * Time.deltaTime);
        }

        void LookAtTarget()
        {
            targetRotation = Quaternion.LookRotation(target.position - transform.position);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, turnSpeed * Time.deltaTime);
        }

        void LookatRotation()
        {
            if (!target) return;

            float wantedRotationAngle = target.eulerAngles.y;
            float wantedHeight = target.position.y + offsetPos.y;

            float currentRotationAngle = transform.eulerAngles.y;
            float currentHeight = transform.position.y;

            currentRotationAngle = Mathf.LerpAngle(currentRotationAngle, wantedRotationAngle, smoothSpeed * Time.deltaTime);

            currentHeight = Mathf.Lerp(currentHeight, wantedHeight, moveSpeed * Time.deltaTime);

            var currentRotation = Quaternion.Euler(0, currentRotationAngle, 0);

            transform.position = target.position;
            transform.position -= currentRotation * Vector3.forward * -offsetPos.z;

            transform.position = new Vector3(transform.position.x, currentHeight, transform.position.z);

            transform.LookAt(target);
        }

        IEnumerator RotateAroundTarget(float angle)
        {
            Vector3 vel = Vector3.zero;
            Vector3 targerOffsetPos = Quaternion.Euler(0, angle, 0) * offsetPos;
            float dist = Vector3.Distance(offsetPos, targerOffsetPos);

            smoothRotating = true;

            while (dist > 0.02f)
            {
                offsetPos = Vector3.SmoothDamp(offsetPos, targerOffsetPos, ref vel, smoothSpeed);
                dist = Vector3.Distance(offsetPos, targerOffsetPos);
                yield return null;
            }

            smoothRotating = false;
            offsetPos = targerOffsetPos;
        }
    }
}
