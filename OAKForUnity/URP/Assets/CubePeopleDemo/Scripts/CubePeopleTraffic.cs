using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AI;

namespace CubePeople
{
    public class CubePeopleTraffic : MonoBehaviour
    {
        NavMeshAgent agent;
        public Vector2 minmaxSpeed = new Vector2(0.5f, 1.5f);

        public int playerState = 0; //0=entry, 1=stay
        public bool refreshDestination = false;
        bool dice;

        public float pauseTime = 1;
        float timeCount;

        //Way point
        public int targetPoint;
        public GameObject destinationFolder;
        List<Transform> wayPoints = new List<Transform>();
        
        //anim
        Animator anim;

        void Start()
        {
            anim = GetComponent<Animator>();
            agent = GetComponent<NavMeshAgent>();
            timeCount = pauseTime;

            if (destinationFolder != null)
            {
                int count = destinationFolder.transform.childCount;
                for (int i = 0; i < count; i++)
                {
                    wayPoints.Add(destinationFolder.transform.GetChild(i));
                }
            }
            else
            {
                print("DestinationFolder is empty, navmesh does not work. (Scene object " + transform.gameObject.name.ToString() + ").");
            }

            agent.speed = RandomSpeed();
            targetPoint = RandomPoint();
            refreshDestination = true;
        }


        void Update()
        {
            if (wayPoints.Count == 0)
            {
                return;
            }
            else
            {
                float dist = Vector3.Distance(wayPoints[targetPoint].position, transform.position);
                if (dist < 0.35f)
                {
                    //arrived
                    if (!dice)
                    {
                        playerState = Random.Range(0, 2);
                        dice = true;
                    }

                    if (playerState == 1)
                    {
                        timeCount -= Time.deltaTime;    //wait
                        if (timeCount < 0)
                        {
                            timeCount = pauseTime;
                            dice = false;
                            playerState = 0;    //return zero
                        }
                    }
                    else
                    {
                        if (dice) dice = false;
                        targetPoint = RandomPoint();    //new point
                        refreshDestination = true;
                    }
                }

                if (refreshDestination)
                {
                    agent.SetDestination(wayPoints[targetPoint].position);
                    refreshDestination = false;
                }
            }
            anim.SetFloat("Walk", agent.velocity.magnitude);
        }

        public int RandomPoint()
        {
            int rPoint = -1;
            if (wayPoints.Count > 0)
            {
                rPoint = Random.Range(0, wayPoints.Count);
                
            }
            return rPoint;
        }

        public float RandomSpeed()
        {
            return Random.Range(minmaxSpeed.x, minmaxSpeed.y);
        }
    }
}
