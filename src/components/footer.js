import React from 'react'
import Link from 'gatsby-link'

const Footer = ({}) => (
  <div
    style={{
      background: '#ffffff',
      height:'100px',
      // marginBottom: '1.45rem',
      position:'absolute',
      width:'100%'
    }}
  >
    <div
      style={{
        // margin: '0 auto',
        // maxWidth: 960,
        // padding: '1rem 1rem',
        display:'flex',
        justifyContent:'center',
        alignItems:'center',
        flexDirection:'row',
      }}
    >

    <Link to="/" style={{ color:'#000000',textDecoration:'none',margin:'20px' }}>
      <img src="/UMBC-horizontal-color.png" style={{ height:'90px',marginBottom:'0px' }} />
    </Link>

    <Link to="/" style={{ color:'#000000',textDecoration:'none',margin:'20px' }}>
      <img src="/retrieverRoboticsBW.png" style={{ height:'90px',width:'90px',marginBottom:'0px' }} />
    </Link>

    </div>
  </div>
)

export default Footer
